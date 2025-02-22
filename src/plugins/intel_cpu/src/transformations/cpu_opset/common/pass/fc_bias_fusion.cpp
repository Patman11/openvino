// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "fc_bias_fusion.hpp"
#include "transformations/cpu_opset/common/op/fully_connected.hpp"
#include <numeric>
#include <ngraph/opsets/opset1.hpp>
#include <ngraph/rt_info.hpp>
#include <ngraph/pattern/op/wrap_type.hpp>

#include "transformations/utils/utils.hpp"

#include "itt.hpp"

ov::intel_cpu::NonQuantizedFullyConnectedBiasFusion::NonQuantizedFullyConnectedBiasFusion() {
    MATCHER_SCOPE(FullyConnectedBiasFusion);
    auto input = ngraph::pattern::any_input();
    auto weights = ngraph::pattern::any_input(ngraph::pattern::has_static_shape());
    auto m_fc = ngraph::pattern::wrap_type<ov::intel_cpu::FullyConnectedNode>({ input, weights }, [](ngraph::Output<ngraph::Node> output) {
        return ngraph::pattern::consumers_count(1)(output) && ngraph::pattern::has_static_rank()(output);
    });
    auto m_bias = ngraph::pattern::any_input(ngraph::pattern::has_static_shape());
    auto m_add = ngraph::pattern::wrap_type<ngraph::opset1::Add>({m_fc, m_bias});

    ngraph::matcher_pass_callback callback = [=](ngraph::pattern::Matcher &m) {
        auto& pattern_to_output = m.get_pattern_value_map();

        auto add = pattern_to_output[m_add].get_node_shared_ptr();
        auto bias = pattern_to_output[m_bias].get_node_shared_ptr();
        auto fc = std::dynamic_pointer_cast<ov::intel_cpu::FullyConnectedNode>(pattern_to_output[m_fc].get_node_shared_ptr());
        if (!fc || transformation_callback(fc)) {
            return false;
        }

        if (!std::dynamic_pointer_cast<ngraph::opset1::Constant>(bias)) {
            return false;
        }

        ngraph::Shape bias_shape(bias->get_shape());
        ngraph::PartialShape output_shape(fc->get_output_partial_shape(0));
        size_t bias_size = ngraph::shape_size(bias_shape);
        auto rank = output_shape.rank().get_length();
        if (rank == 0 || output_shape[rank - 1].is_dynamic()) {
            return false;
        }

        if (bias_shape.empty() || static_cast<int64_t>(bias_shape.back()) != output_shape[rank - 1].get_length() ||
            bias_shape.back() != bias_size) {
            return false;
        }

        ngraph::NodeVector new_ops;

        std::shared_ptr<ngraph::Node> final_bias = bias;
        if (bias_shape.size() >= 2) {
            auto reshape_const = ngraph::opset1::Constant::create(ngraph::element::i64, ngraph::Shape{ 1 }, { -1 });
            final_bias = ov::op::util::make_try_fold<ngraph::opset1::Reshape>(final_bias, reshape_const, true);
            new_ops.push_back(final_bias);
        }

        auto new_fc = std::make_shared<ov::intel_cpu::FullyConnectedNode>(fc->input_value(0),
                                                                         fc->input_value(1),
                                                                         final_bias,
                                                                         fc->get_output_rank(),
                                                                         fc->get_output_type());
        new_ops.push_back(new_fc);

        new_fc->set_friendly_name(add->get_friendly_name());
        ngraph::copy_runtime_info({fc, add}, new_ops);
        ngraph::replace_node(add, new_fc);
        return true;
    };

    auto m = std::make_shared<ngraph::pattern::Matcher>(m_add, matcher_name);
    this->register_matcher(m, callback);
}

//CPU plugin would config LPT  not to propagate dequantization scale over bias to follow ONEDNN 3.x scheme.
//It is a little tricky now to first fuse bias not DQ for pattern "FC + DQ + BIAS".
//todo: Will move the FullyConnect fusing into CPU and fuse the DQ and BIAS in topology order.
ov::intel_cpu::QuantizedFullyConnectedBiasFusion::QuantizedFullyConnectedBiasFusion() {
    MATCHER_SCOPE(FullyConnectedBiasFusion);
    auto input = ngraph::pattern::any_input();
    auto weights = ngraph::pattern::any_input(ngraph::pattern::has_static_shape());
    auto m_fc = ngraph::pattern::wrap_type<ov::intel_cpu::FullyConnectedNode>({ input, weights }, [](ngraph::Output<ngraph::Node> output) {
        return ngraph::pattern::consumers_count(1)(output) && ngraph::pattern::has_static_rank()(output);
    });

    auto m_scale = ngraph::pattern::any_input(ngraph::pattern::has_static_shape());
    auto m_mul = ngraph::pattern::wrap_type<ngraph::opset1::Multiply>({m_fc, m_scale});

    auto m_bias = ngraph::pattern::any_input(ngraph::pattern::has_static_shape());
    auto m_add = ngraph::pattern::wrap_type<ngraph::opset1::Add>({m_mul, m_bias});

    ngraph::matcher_pass_callback callback = [=](ngraph::pattern::Matcher &m) {
        auto& pattern_to_output = m.get_pattern_value_map();
        auto mul = pattern_to_output[m_mul].get_node_shared_ptr();
        auto scale = pattern_to_output[m_scale].get_node_shared_ptr();
        auto add = pattern_to_output[m_add].get_node_shared_ptr();
        auto bias = pattern_to_output[m_bias].get_node_shared_ptr();
        auto fc = std::dynamic_pointer_cast<ov::intel_cpu::FullyConnectedNode>(pattern_to_output[m_fc].get_node_shared_ptr());
        if (!fc || transformation_callback(fc)) {
            return false;
        }

        if (!std::dynamic_pointer_cast<ngraph::opset1::Constant>(bias)) {
            return false;
        }

        ngraph::Shape bias_shape(bias->get_shape());
        ngraph::PartialShape output_shape(fc->get_output_partial_shape(0));
        size_t bias_size = ngraph::shape_size(bias_shape);
        auto rank = output_shape.rank().get_length();
        if (rank == 0 || output_shape[rank - 1].is_dynamic()) {
            return false;
        }

        const bool per_channel = std::count_if(bias_shape.begin(), bias_shape.end(), [](size_t x) { return x > 1; }) == 1;
        if (ov::shape_size(bias_shape) != 1 && !per_channel)
            return false;

        if (bias_shape.empty() || static_cast<int64_t>(bias_shape.back()) != output_shape[rank - 1].get_length() ||
            bias_shape.back() != bias_size) {
            return false;
        }

        ngraph::NodeVector new_ops;

        std::shared_ptr<ngraph::Node> final_bias = bias;
        if (bias_shape.size() >= 2) {
            auto reshape_const = ngraph::opset1::Constant::create(ngraph::element::i64, ngraph::Shape{ 1 }, { -1 });
            final_bias = ov::op::util::make_try_fold<ngraph::opset1::Reshape>(final_bias, reshape_const, true);
            new_ops.push_back(final_bias);
        }

        auto new_fc = std::make_shared<ov::intel_cpu::FullyConnectedNode>(fc->input_value(0),
                                                                         fc->input_value(1),
                                                                         final_bias,
                                                                         fc->get_output_rank(),
                                                                         fc->get_output_type());
        new_ops.push_back(new_fc);

        std::shared_ptr<ngraph::Node> final_scale = scale;
        auto new_mul = std::make_shared<ngraph::opset1::Multiply>(new_fc, final_scale, mul->get_autob());
        new_ops.push_back(new_mul);

        new_mul->set_friendly_name(add->get_friendly_name());
        ngraph::copy_runtime_info({fc, mul, add}, new_ops);
        ngraph::replace_node(add, new_mul);
        return true;
    };

    auto m = std::make_shared<ngraph::pattern::Matcher>(m_add, matcher_name);
    this->register_matcher(m, callback);
}