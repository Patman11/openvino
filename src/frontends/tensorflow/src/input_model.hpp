// Copyright (C) 2018-2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "openvino/frontend/extension/telemetry.hpp"
#include "openvino/frontend/graph_iterator.hpp"
#include "openvino/frontend/input_model.hpp"
#include "place.hpp"
#include "translate_session.hpp"

namespace ov {
namespace frontend {
namespace tensorflow {

class OpPlace;
class TensorPlace;
class VariablesIndex;

class InputModel : public ov::frontend::InputModel {
    friend class TranslateSession;
    class InputModelTFImpl;
    std::shared_ptr<InputModelTFImpl> _impl;

    std::vector<std::string> get_input_names() const;
    std::vector<std::string> get_output_names() const;
    std::vector<std::shared_ptr<OpPlace>> get_op_places() const;
    std::map<std::string, Output<Node>> get_tensor_values() const;
    std::shared_ptr<InputModel> get_body_input_model(const std::string& body_input_model_name) const;

public:
    explicit InputModel(const GraphIterator::Ptr& graph_iterator,
                        const std::shared_ptr<TelemetryExtension>& telemetry = {},
                        const std::shared_ptr<VariablesIndex>& variables_index = {},
                        const std::shared_ptr<std::map<std::string, std::string>> saved_model_input_names = nullptr,
                        const std::shared_ptr<std::map<std::string, std::string>> saved_model_output_names = nullptr,
                        const bool native_format = false);

    std::vector<ov::frontend::Place::Ptr> get_inputs() const override;
    std::vector<ov::frontend::Place::Ptr> get_outputs() const override;
    ov::frontend::Place::Ptr get_place_by_tensor_name(const std::string& tensorName) const override;
    void override_all_outputs(const std::vector<ov::frontend::Place::Ptr>& outputs) override;
    void override_all_inputs(const std::vector<ov::frontend::Place::Ptr>& inputs) override;
    void extract_subgraph(const std::vector<ov::frontend::Place::Ptr>& inputs,
                          const std::vector<ov::frontend::Place::Ptr>& outputs) override;
    void set_partial_shape(const ov::frontend::Place::Ptr& place, const ov::PartialShape&) override;
    ov::PartialShape get_partial_shape(const ov::frontend::Place::Ptr& place) const override;
    void set_element_type(const ov::frontend::Place::Ptr& place, const ov::element::Type&) override;
    ov::element::Type get_element_type(const ov::frontend::Place::Ptr& place) const override;
    void set_tensor_value(const ov::frontend::Place::Ptr& place, const void* value) override;
    std::shared_ptr<VariablesIndex> get_variables_index();
    std::shared_ptr<std::map<std::string, std::string>> get_saved_model_input_names() const;
    std::shared_ptr<std::map<std::string, std::string>> get_saved_model_output_names() const;

    std::map<std::string, std::shared_ptr<TensorPlace>> get_tensor_places() const;
};

}  // namespace tensorflow
}  // namespace frontend
}  // namespace ov
