// Copyright (C) 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "snippets/op/brgemm.hpp"
#include "brgemm_copy_b.hpp"

#include "snippets/lowered/port_descriptor.hpp"

namespace ov {
namespace intel_cpu {

/**
 * @interface BrgemmCPU
 * @brief BrgemmCPU is a batch-reduced matrix multiplication with the support of arbitrary strides between matrices rows
 *        with support of several precisions on plugin level
 * @ingroup snippets
 */
class BrgemmCPU : public snippets::op::Brgemm {
public:
    OPENVINO_OP("BrgemmCPU", "SnippetsOpset", snippets::op::Brgemm);

    enum Type {
        Floating,          // f32|f32
        WithDataRepacking, // u8|i8 or bf16|bf16 (non-AMX system) - needs BrgemmCopyB on second input for data repacking
        WithCompensations, // i8|i8 (non-AMX system) - needs BrgemmCopyB for data repacking and compensations
        AMX,               // i8|i8 or bf16|bf16 on AMX system - needs BrgemmCopyB and scratchpad
    };

    BrgemmCPU(const Output<Node>& A, const Output<Node>& B, const Type type,
              const size_t offset_a = 0, const size_t offset_b = 0, const size_t offset_c = 0,
              std::vector<size_t> layout_a = {}, std::vector<size_t> layout_b = {}, std::vector<size_t> layout_c = {});
    BrgemmCPU(const Output<Node>& A, const Output<Node>& B, const Output<Node>& scratch, const Type type,
              const size_t offset_a = 0, const size_t offset_b = 0, const size_t offset_scratch = 0, const size_t offset_c = 0,
              std::vector<size_t> layout_a = {}, std::vector<size_t> layout_b = {}, std::vector<size_t> layout_c = {});
    BrgemmCPU() = default;

    void validate_and_infer_types() override;
    std::shared_ptr<Node> clone_with_new_inputs(const OutputVector& new_args) const override;

    Type get_type() const { return m_type; }
    bool is_with_compensations() const { return m_type == Type::WithCompensations; }
    bool is_with_data_repacking() const { return m_type != Type::Floating; }
    bool is_amx() const { return m_type == Type::AMX; }
    bool is_with_scratchpad() const { return is_with_compensations() || is_amx(); }

    size_t get_offset_scratch() const;
    std::shared_ptr<BrgemmCopyB> get_brgemm_copy() const;

    constexpr static size_t SCRATCH_BYTE_SIZE = 32 * 1024;

private:
    void custom_constructor_validate_and_infer_types(std::vector<size_t> layout_a, std::vector<size_t> layout_b, std::vector<size_t> layout_c);
    void validate_with_scratchpad(const ov::Shape& shape_b) const;
    void validate_inputs() const;

    Type m_type = Type::Floating;
};

} // namespace intel_cpu
} // namespace ov
