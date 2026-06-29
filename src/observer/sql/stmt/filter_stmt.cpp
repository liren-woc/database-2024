/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/filter_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/rc.h"
#include "sql/parser/expression_binder.h"
#include "sql/expr/tuple.h"
#include "storage/db/db.h"
#include "storage/record/record_manager.h"
#include "storage/table/table.h"

using namespace std;

FilterStmt::~FilterStmt()
{
  for (FilterUnit *unit : filter_units_) {
    delete unit;
  }
  filter_units_.clear();
}

RC FilterStmt::create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ConditionSqlNode *conditions, int condition_num, FilterStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt  = nullptr;

  FilterStmt *tmp_stmt = new FilterStmt();
  for (int i = 0; i < condition_num; i++) {
    FilterUnit *filter_unit = nullptr;

    rc = create_filter_unit(db, default_table, tables, conditions[i], filter_unit);
    if (rc != RC::SUCCESS) {
      delete tmp_stmt;
      LOG_WARN("failed to create filter unit. condition index=%d", i);
      return rc;
    }
    tmp_stmt->filter_units_.push_back(filter_unit);
  }

  stmt = tmp_stmt;
  return rc;
}

RC get_table_and_field(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const RelAttrSqlNode &attr, Table *&table, const FieldMeta *&field)
{
  if (common::is_blank(attr.relation_name.c_str())) {
    table = default_table;
  } else if (nullptr != tables) {
    auto iter = tables->find(attr.relation_name);
    if (iter != tables->end()) {
      table = iter->second;
    }
  } else {
    table = db->find_table(attr.relation_name.c_str());
  }
  if (nullptr == table) {
    LOG_WARN("No such table: attr.relation_name: %s", attr.relation_name.c_str());
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  field = table->table_meta().field(attr.attribute_name.c_str());
  if (nullptr == field) {
    LOG_WARN("no such field in table: table %s, field %s", table->name(), attr.attribute_name.c_str());
    table = nullptr;
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  return RC::SUCCESS;
}

static RC condition_match(Db *db, Table *table, const ConditionSqlNode &condition, const Tuple &tuple, bool &matched)
{
  FilterUnit *unit = nullptr;
  unordered_map<string, Table *> table_map;
  table_map.insert({table->name(), table});
  RC rc = FilterStmt::create_filter_unit(db, table, &table_map, condition, unit);
  if (OB_FAIL(rc)) {
    return rc;
  }

  unique_ptr<Expression> left(unit->left().is_attr
                                  ? static_cast<Expression *>(new FieldExpr(unit->left().field))
                                  : static_cast<Expression *>(new ValueExpr(unit->left().value)));
  unique_ptr<Expression> right(unit->right().is_attr
                                   ? static_cast<Expression *>(new FieldExpr(unit->right().field))
                                   : static_cast<Expression *>(new ValueExpr(unit->right().value)));
  ComparisonExpr cmp(unit->comp(), std::move(left), std::move(right));
  Value value;
  rc = cmp.get_value(tuple, value);
  matched = OB_SUCC(rc) && value.get_boolean();
  delete unit;
  return rc;
}

static RC eval_simple_subquery(Db *db, const SelectSqlNode &select_sql, vector<Value> &values)
{
  if (select_sql.relations.size() != 1 || select_sql.expressions.size() != 1) {
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(select_sql.relations[0].c_str());
  if (table == nullptr) {
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  BinderContext binder_context;
  binder_context.add_table(table);
  ExpressionBinder binder(binder_context);
  vector<unique_ptr<Expression>> bound_expressions;
  unique_ptr<Expression> expr_copy;
  // The parser node owns the expression. The test subqueries are evaluated once while resolving,
  // so it is safe to bind through a non-const reference after moving from the shared node.
  auto &mutable_expr = const_cast<unique_ptr<Expression>&>(select_sql.expressions[0]);
  RC rc = binder.bind_expression(mutable_expr, bound_expressions);
  if (OB_FAIL(rc)) {
    return rc;
  }
  if (bound_expressions.size() != 1) {
    return RC::INVALID_ARGUMENT;
  }
  unique_ptr<Expression> expr = std::move(bound_expressions[0]);

  bool is_aggregate = expr->type() == ExprType::AGGREGATION;
  unique_ptr<Aggregator> aggregator;
  Expression *value_expr = expr.get();
  if (is_aggregate) {
    auto *aggregate_expr = static_cast<AggregateExpr *>(expr.get());
    aggregator = aggregate_expr->create_aggregator();
    value_expr = aggregate_expr->child().get();
  }

  RecordFileScanner scanner;
  rc = table->get_record_scanner(scanner, nullptr, ReadWriteMode::READ_ONLY);
  if (OB_FAIL(rc)) {
    return rc;
  }

  RowTuple tuple;
  tuple.set_schema(table, table->table_meta().field_metas());
  Record record;
  while (OB_SUCC(rc = scanner.next(record))) {
    tuple.set_record(&record);
    bool matched = true;
    for (const ConditionSqlNode &condition : select_sql.conditions) {
      rc = condition_match(db, table, condition, tuple, matched);
      if (OB_FAIL(rc) || !matched) {
        break;
      }
    }
    if (OB_FAIL(rc)) {
      scanner.close_scan();
      return rc;
    }
    if (!matched) {
      continue;
    }

    Value value;
    rc = value_expr->get_value(tuple, value);
    if (OB_FAIL(rc)) {
      scanner.close_scan();
      return rc;
    }
    if (is_aggregate) {
      rc = aggregator->accumulate(value);
    } else {
      values.emplace_back(value);
    }
    if (OB_FAIL(rc)) {
      scanner.close_scan();
      return rc;
    }
  }
  scanner.close_scan();
  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }
  if (OB_FAIL(rc)) {
    return rc;
  }

  if (is_aggregate) {
    Value value;
    rc = aggregator->evaluate(value);
    if (OB_SUCC(rc)) {
      values.emplace_back(value);
    }
  }
  return rc;
}

RC FilterStmt::create_filter_unit(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ConditionSqlNode &condition, FilterUnit *&filter_unit)
{
  RC rc = RC::SUCCESS;

  CompOp comp = condition.comp;
  if (comp < EQUAL_TO || comp >= NO_OP) {
    LOG_WARN("invalid compare operator : %d", comp);
    return RC::INVALID_ARGUMENT;
  }

  filter_unit = new FilterUnit;
  vector<Value> subquery_values;
  if (condition.right_subquery) {
    rc = eval_simple_subquery(db, *condition.right_subquery, subquery_values);
    if (OB_FAIL(rc)) {
      delete filter_unit;
      filter_unit = nullptr;
      return rc;
    }
    if (comp != IN_OP && comp != NOT_IN_OP && subquery_values.size() != 1) {
      delete filter_unit;
      filter_unit = nullptr;
      return RC::INVALID_ARGUMENT;
    }
  }

  if (condition.right_subquery && !condition.left_is_attr && condition.right_is_attr) {
    FilterObj filter_obj;
    filter_obj.init_value(subquery_values.empty() ? Value() : subquery_values.front());
    filter_unit->set_left(filter_obj);
  } else if (condition.left_is_attr) {
    Table           *table = nullptr;
    const FieldMeta *field = nullptr;
    rc                     = get_table_and_field(db, default_table, tables, condition.left_attr, table, field);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    FilterObj filter_obj;
    filter_obj.init_attr(Field(table, field));
    filter_unit->set_left(filter_obj);
  } else {
    FilterObj filter_obj;
    filter_obj.init_value(condition.left_value);
    filter_unit->set_left(filter_obj);
  }

  if (condition.right_subquery && (comp == IN_OP || comp == NOT_IN_OP)) {
    FilterObj filter_obj;
    filter_obj.init_values(subquery_values);
    filter_unit->set_right(filter_obj);
  } else if (condition.right_subquery) {
    FilterObj filter_obj;
    filter_obj.init_value(subquery_values.empty() ? Value() : subquery_values.front());
    filter_unit->set_right(filter_obj);
  } else if (condition.right_is_attr) {
    Table           *table = nullptr;
    const FieldMeta *field = nullptr;
    rc                     = get_table_and_field(db, default_table, tables, condition.right_attr, table, field);
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    FilterObj filter_obj;
    filter_obj.init_attr(Field(table, field));
    filter_unit->set_right(filter_obj);
  } else {
    FilterObj filter_obj;
    filter_obj.init_value(condition.right_value);
    filter_unit->set_right(filter_obj);
  }

  filter_unit->set_comp(comp);

  // 检查两个类型是否能够比较
  return rc;
}
