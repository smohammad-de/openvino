// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <memory>
#include <vector>

#include <ngraph/ngraph.hpp>
#include "ngraph/opsets/opset1.hpp"
#include "snippets/op/convert_saturation.hpp"
#include "snippets_helpers.hpp"

namespace ov {
namespace test {
namespace snippets {

/**
 * @class DummyAdd
 * @brief DummyAdd operation has custom validate_and_infer_types method implementation.
 */
class DummyAdd : public ngraph::opset1::Add {
public:
    OPENVINO_OP("DummyAdd", "test::snippets");

    DummyAdd(const Output<Node>& arg0,
        const Output<Node>& arg1,
        const ngraph::op::AutoBroadcastSpec& auto_broadcast =
        ngraph::op::AutoBroadcastSpec(ngraph::op::AutoBroadcastType::NUMPY))
        : ngraph::opset1::Add(arg0, arg1, auto_broadcast) {
        constructor_validate_and_infer_types();
    }

    DummyAdd(const ngraph::opset1::Add& add)
        : Add(add.get_input_source_output(0), add.get_input_source_output(1), add.get_autob()) {
        constructor_validate_and_infer_types();
    }

    DummyAdd() = default;

    void validate_and_infer_types() override {
        const auto input_type1 = get_input_element_type(0);
        const auto input_type2 = get_input_element_type(1);

        const element::Type output_type = (input_type1 == element::i8) || (input_type2 == element::i8) ?
            element::i32 :
            get_input_element_type(0);

        set_output_type(0, output_type, get_input_partial_shape(0));
    }

    std::shared_ptr<Node> clone_with_new_inputs(const OutputVector& new_args) const override {
        return std::make_shared<DummyAdd>(new_args.at(0), new_args.at(1), this->get_autob());
    }
};

class PrecisionPropagationAddFunctionParams {
public:
    class Actual {
    public:
        std::pair<element::Type, element::Type> convertion_before_op1;
        element::Type convertion_before_op2_1;
        std::pair<element::Type, element::Type> convertion_before_op2_2;
    };

    class Expected {
    public:
        std::pair<element::Type, element::Type> convertion_before_op1;
        element::Type convertion_before_op2_1;
        std::pair<element::Type, element::Type> convertion_before_op2_2;
        element::Type convertion_after_op2;
    };
};

/**
 * @class PrecisionPropagationAddFunction
 * @brief PrecisionPropagationAddFunction instance returns reference and original functions.
 *
 * Input arguments are used to create function in getOriginal or getReference methods only.
 * Dont use getLowered method, it is not implemented and throw std::runtime_error exception.
 * Note, ov::element::Type_t precision base type input argument is not used.
 */
class PrecisionPropagationAddFunction : public SnippetsFunctionBase {
public:
    explicit PrecisionPropagationAddFunction(
        const std::vector<PartialShape> input_shapes,
        const ngraph::element::Type precision1,
        const ngraph::element::Type precision2,
        const ngraph::element::Type constant_precision,
        PrecisionPropagationAddFunctionParams::Actual actual,
        PrecisionPropagationAddFunctionParams::Expected expected) :
        SnippetsFunctionBase(input_shapes),
        precision1(precision1),
        precision2(precision2),
        constant_precision(constant_precision),
        actual(actual),
        expected(expected) {
        OPENVINO_ASSERT(input_shapes.size() == 2ull, "input_shapes size has to be equal to 2");
    }

    /*
     * Don't call this method explicity. You should create the instance of PrecisionPropagationAddFunction before.
     * After the method will be called implicitly in getOriginal or getReference methods.
     * Note, please, getLowered method is not implemented and throws exception.
     */
    static std::shared_ptr<ngraph::Function> get(
        const ngraph::element::Type precision1,
        const ngraph::PartialShape& inputShape1,
        const ngraph::element::Type precision2,
        const ngraph::PartialShape& inputShape2,
        const ngraph::element::Type constant_precision,
        const std::pair<element::Type, element::Type>& convertion_before_op1 = std::pair<element::Type, element::Type>(),
        const element::Type convertion_before_op2_1 = element::undefined,
        const std::pair<element::Type, element::Type>& convertion_before_op2_2 = std::pair<element::Type, element::Type>(),
        const element::Type convertion_after_op2 = {});

protected:
    std::shared_ptr<ov::Model> initOriginal() const override;
    std::shared_ptr<ov::Model> initReference() const override;

    const ngraph::element::Type precision1;
    const ngraph::element::Type precision2;
    const ngraph::element::Type constant_precision;
    const PrecisionPropagationAddFunctionParams::Actual actual;
    const PrecisionPropagationAddFunctionParams::Expected expected;
};

}  // namespace snippets
}  // namespace test
}  // namespace ov
