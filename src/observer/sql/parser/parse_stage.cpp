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
// Created by Longda on 2021/4/13.
//

#include <ctype.h>
#include <string.h>
#include <cstdlib>
#include <string>
#include <vector>

#include "parse_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/parser/parse.h"

using namespace common;

namespace {
bool is_identifier(const std::string &text)
{
  if (text.empty()) {
    return false;
  }

  if (!(isalpha(static_cast<unsigned char>(text[0])) || text[0] == '_')) {
    return false;
  }

  for (size_t i = 1; i < text.size(); i++) {
    if (!(isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) {
      return false;
    }
  }
  return true;
}

void skip_spaces(const std::string &sql, size_t &pos)
{
  while (pos < sql.size() && isspace(static_cast<unsigned char>(sql[pos]))) {
    pos++;
  }
}

bool consume_keyword(const std::string &sql, size_t &pos, const char *keyword)
{
  skip_spaces(sql, pos);

  const size_t len = strlen(keyword);
  if (pos + len > sql.size()) {
    return false;
  }

  for (size_t i = 0; i < len; i++) {
    if (tolower(static_cast<unsigned char>(sql[pos + i])) !=
        tolower(static_cast<unsigned char>(keyword[i]))) {
      return false;
    }
  }

  if (pos + len < sql.size()) {
    const char next = sql[pos + len];
    if (isalnum(static_cast<unsigned char>(next)) || next == '_') {
      return false;
    }
  }

  pos += len;
  return true;
}

bool parse_identifier_token(const std::string &sql, size_t &pos, std::string &identifier)
{
  skip_spaces(sql, pos);
  const size_t begin = pos;
  if (begin >= sql.size()) {
    return false;
  }

  if (!(isalpha(static_cast<unsigned char>(sql[pos])) || sql[pos] == '_')) {
    return false;
  }
  pos++;
  while (pos < sql.size() && (isalnum(static_cast<unsigned char>(sql[pos])) || sql[pos] == '_')) {
    pos++;
  }

  identifier = sql.substr(begin, pos - begin);
  return true;
}

bool parse_string_literal(const std::string &sql, size_t &pos, Value &value)
{
  if (pos >= sql.size() || sql[pos] != '\'') {
    return false;
  }

  pos++;
  std::string text;
  while (pos < sql.size()) {
    const char ch = sql[pos++];
    if (ch == '\'') {
      if (pos < sql.size() && sql[pos] == '\'') {
        text.push_back('\'');
        pos++;
        continue;
      }
      value = Value(text.c_str());
      return true;
    }
    text.push_back(ch);
  }
  return false;
}

bool parse_number_literal(const std::string &sql, size_t &pos, Value &value)
{
  const size_t begin = pos;
  if (pos < sql.size() && (sql[pos] == '+' || sql[pos] == '-')) {
    pos++;
  }

  bool has_digit = false;
  while (pos < sql.size() && isdigit(static_cast<unsigned char>(sql[pos]))) {
    has_digit = true;
    pos++;
  }

  bool is_float = false;
  if (pos < sql.size() && sql[pos] == '.') {
    is_float = true;
    pos++;
    while (pos < sql.size() && isdigit(static_cast<unsigned char>(sql[pos]))) {
      has_digit = true;
      pos++;
    }
  }

  if (!has_digit) {
    pos = begin;
    return false;
  }

  if (pos < sql.size() && (sql[pos] == 'e' || sql[pos] == 'E')) {
    is_float = true;
    size_t exp_pos = pos + 1;
    if (exp_pos < sql.size() && (sql[exp_pos] == '+' || sql[exp_pos] == '-')) {
      exp_pos++;
    }
    bool has_exp_digit = false;
    while (exp_pos < sql.size() && isdigit(static_cast<unsigned char>(sql[exp_pos]))) {
      has_exp_digit = true;
      exp_pos++;
    }
    if (!has_exp_digit) {
      pos = begin;
      return false;
    }
    pos = exp_pos;
  }

  const std::string token = sql.substr(begin, pos - begin);
  if (is_float) {
    value = Value(static_cast<float>(strtod(token.c_str(), nullptr)));
  } else {
    value = Value(static_cast<int>(strtol(token.c_str(), nullptr, 10)));
  }
  return true;
}

bool parse_insert_value(const std::string &sql, size_t &pos, Value &value)
{
  skip_spaces(sql, pos);
  if (pos >= sql.size()) {
    return false;
  }

  if (sql[pos] == '\'') {
    return parse_string_literal(sql, pos, value);
  }

  return parse_number_literal(sql, pos, value);
}

bool parse_value_group(const std::string &sql, size_t &pos, std::vector<Value> &values)
{
  skip_spaces(sql, pos);
  if (pos >= sql.size() || sql[pos] != '(') {
    return false;
  }
  pos++;

  while (true) {
    Value value;
    if (!parse_insert_value(sql, pos, value)) {
      return false;
    }
    values.emplace_back(std::move(value));

    skip_spaces(sql, pos);
    if (pos >= sql.size()) {
      return false;
    }

    if (sql[pos] == ',') {
      pos++;
      continue;
    }
    if (sql[pos] == ')') {
      pos++;
      return true;
    }
    return false;
  }
}

bool try_parse_insert(const std::string &sql, ParsedSqlResult &parsed_sql_result)
{
  size_t pos = 0;
  if (!consume_keyword(sql, pos, "insert")) {
    return false;
  }
  if (!consume_keyword(sql, pos, "into")) {
    return false;
  }

  std::string table_name;
  if (!parse_identifier_token(sql, pos, table_name) || !is_identifier(table_name)) {
    return false;
  }

  if (!consume_keyword(sql, pos, "values")) {
    return false;
  }

  auto sql_node = std::make_unique<ParsedSqlNode>(SCF_INSERT);
  sql_node->insertion.relation_name = std::move(table_name);

  while (true) {
    std::vector<Value> value_group;
    if (!parse_value_group(sql, pos, value_group)) {
      return false;
    }
    sql_node->insertion.value_groups.emplace_back(std::move(value_group));

    skip_spaces(sql, pos);
    if (pos >= sql.size()) {
      break;
    }
    if (sql[pos] == ',') {
      pos++;
      continue;
    }
    if (sql[pos] == ';') {
      pos++;
      break;
    }
    return false;
  }

  skip_spaces(sql, pos);
  if (pos != sql.size()) {
    return false;
  }
  if (sql_node->insertion.value_groups.empty()) {
    return false;
  }

  parsed_sql_result.add_sql_node(std::move(sql_node));
  return true;
}


bool try_parse_create_unique_index(const std::string &sql, ParsedSqlResult &parsed_sql_result)
{
  size_t pos = 0;
  if (!consume_keyword(sql, pos, "create")) {
    return false;
  }
  if (!consume_keyword(sql, pos, "unique")) {
    return false;
  }
  if (!consume_keyword(sql, pos, "index")) {
    return false;
  }

  std::string index_name;
  if (!parse_identifier_token(sql, pos, index_name) || !is_identifier(index_name)) {
    return false;
  }
  if (!consume_keyword(sql, pos, "on")) {
    return false;
  }

  std::string table_name;
  if (!parse_identifier_token(sql, pos, table_name) || !is_identifier(table_name)) {
    return false;
  }

  skip_spaces(sql, pos);
  if (pos >= sql.size() || sql[pos] != '(') {
    return false;
  }
  pos++;

  std::string attribute_name;
  if (!parse_identifier_token(sql, pos, attribute_name) || !is_identifier(attribute_name)) {
    return false;
  }

  skip_spaces(sql, pos);
  if (pos >= sql.size() || sql[pos] != ')') {
    return false;
  }
  pos++;
  skip_spaces(sql, pos);
  if (pos < sql.size() && sql[pos] == ';') {
    pos++;
  }
  skip_spaces(sql, pos);
  if (pos != sql.size()) {
    return false;
  }

  auto sql_node = std::make_unique<ParsedSqlNode>(SCF_CREATE_INDEX);
  sql_node->create_index.unique = true;
  sql_node->create_index.index_name = std::move(index_name);
  sql_node->create_index.relation_name = std::move(table_name);
  sql_node->create_index.attribute_name = std::move(attribute_name);
  parsed_sql_result.add_sql_node(std::move(sql_node));
  return true;
}

bool try_parse_show_index(const std::string &sql, ParsedSqlResult &parsed_sql_result)
{
  std::string normalized;
  normalized.reserve(sql.size());
  for (char ch : sql) {
    normalized.push_back(static_cast<char>(tolower(static_cast<unsigned char>(ch))));
  }

  size_t pos = 0;
  while (pos < normalized.size() && isspace(static_cast<unsigned char>(normalized[pos]))) {
    pos++;
  }

  const std::string prefix = "show index from ";
  if (normalized.compare(pos, prefix.size(), prefix) != 0) {
    return false;
  }
  pos += prefix.size();

  size_t name_begin = pos;
  while (pos < normalized.size() && !isspace(static_cast<unsigned char>(normalized[pos])) && normalized[pos] != ';') {
    pos++;
  }
  if (name_begin == pos) {
    return false;
  }

  std::string table_name = sql.substr(name_begin, pos - name_begin);
  if (!is_identifier(table_name)) {
    return false;
  }

  while (pos < normalized.size() && isspace(static_cast<unsigned char>(normalized[pos]))) {
    pos++;
  }
  if (pos < normalized.size() && normalized[pos] == ';') {
    pos++;
  }
  while (pos < normalized.size() && isspace(static_cast<unsigned char>(normalized[pos]))) {
    pos++;
  }
  if (pos != normalized.size()) {
    return false;
  }

  auto sql_node = std::make_unique<ParsedSqlNode>(SCF_SHOW_INDEX);
  sql_node->show_index.relation_name = std::move(table_name);
  parsed_sql_result.add_sql_node(std::move(sql_node));
  return true;
}
}  // namespace

RC ParseStage::handle_request(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;

  SqlResult         *sql_result = sql_event->session_event()->sql_result();
  const std::string &sql        = sql_event->sql();

  ParsedSqlResult parsed_sql_result;

  if (!try_parse_show_index(sql, parsed_sql_result) && !try_parse_insert(sql, parsed_sql_result) &&
      !try_parse_create_unique_index(sql, parsed_sql_result)) {
    parse(sql.c_str(), &parsed_sql_result);
  }
  if (parsed_sql_result.sql_nodes().empty()) {
    sql_result->set_return_code(RC::SUCCESS);
    sql_result->set_state_string("");
    return RC::INTERNAL;
  }

  if (parsed_sql_result.sql_nodes().size() > 1) {
    LOG_WARN("got multi sql commands but only 1 will be handled");
  }

  std::unique_ptr<ParsedSqlNode> sql_node = std::move(parsed_sql_result.sql_nodes().front());
  if (sql_node->flag == SCF_ERROR) {
    rc = RC::SQL_SYNTAX;
    sql_result->set_return_code(rc);
    sql_result->set_state_string("FAILURE");
    return rc;
  }

  sql_event->set_sql_node(std::move(sql_node));

  return RC::SUCCESS;
}
