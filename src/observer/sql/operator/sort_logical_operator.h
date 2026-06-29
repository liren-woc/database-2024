/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "sql/operator/logical_operator.h"

class SortLogicalOperator : public LogicalOperator
{
public:
  SortLogicalOperator(std::vector<std::unique_ptr<Expression>> &&expressions, std::vector<bool> &&asc)
      : asc_(std::move(asc))
  {
    expressions_ = std::move(expressions);
  }

  LogicalOperatorType type() const override { return LogicalOperatorType::SORT; }
  std::vector<bool>  &asc() { return asc_; }

private:
  std::vector<bool> asc_;
};
