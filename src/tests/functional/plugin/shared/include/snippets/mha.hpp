// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "shared_test_classes/base/snippets_test_utils.hpp"

namespace ov {
namespace test {
namespace snippets {

typedef std::tuple<
        std::vector<ov::PartialShape>,     // Input shapes
        std::vector<ov::element::Type>,    // Input Element types
        ov::element::Type,                 // Inference precision
        bool,                              // With Multiply
        size_t,                            // Expected num nodes
        size_t,                            // Expected num subgraphs
        std::string,                       // Target Device
        std::map<std::string, std::string> // Config
> MHAParams;

class MHA : public testing::WithParamInterface<ov::test::snippets::MHAParams>,
            virtual public ov::test::SnippetsTestsCommon {
public:
    static std::string getTestCaseName(testing::TestParamInfo<ov::test::snippets::MHAParams> obj);

protected:
    void SetUp() override;

    void generate_inputs(const std::vector<ngraph::Shape>& targetInputStaticShapes) override;
    virtual void init_subgraph();

    bool m_with_mul = false;
    std::vector<ov::element::Type> m_input_types;
};

class MHASelect : public MHA {
protected:
    void generate_inputs(const std::vector<ngraph::Shape>& targetInputStaticShapes) override;
    void init_subgraph() override;
};

class MHAWOTransposeOnInputs : public MHA {
protected:
    void init_subgraph() override;
};

class MHAWOTranspose : public MHA {
protected:
    void init_subgraph() override;
};

class MHAINT8MatMul : public MHA {
protected:
    void init_subgraph() override;
};

class MHAFQAfterMatMul : public MHA {
protected:
    void init_subgraph() override;
};

class MHAFQ : public MHA {
protected:
    void init_subgraph() override;
};

class MHAMulAdd : public MHA {
    void init_subgraph() override;
};


} // namespace snippets
} // namespace test
} // namespace ov
