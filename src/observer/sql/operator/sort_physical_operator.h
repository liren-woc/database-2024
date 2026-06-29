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

#include "sql/operator/physical_operator.h"

class SortPhysicalOperator : public PhysicalOperator
{
public:
  SortPhysicalOperator(std::vector<std::unique_ptr<Expression>> &&expressions, std::vector<bool> &&asc);

  PhysicalOperatorType type() const override { return PhysicalOperatorType::SORT; }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;
  Tuple *current_tuple() override;
  RC tuple_schema(TupleSchema &schema) const override;

private:
  struct SortItem
  {
    ValueListTuple     tuple;
    std::vector<Value> keys;
  };

  std::vector<std::unique_ptr<Expression>> expressions_;
  std::vector<bool>                        asc_;
  std::vector<SortItem>                    tuples_;
  int                                      tuple_index_ = -1;
  TupleSchema                              schema_;
};
