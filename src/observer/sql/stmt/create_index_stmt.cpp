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
// Created by Wangyunlai on 2023/4/25.
//

#include "sql/stmt/create_index_stmt.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"

using namespace std;
using namespace common;

RC CreateIndexStmt::create(Db *db, const CreateIndexSqlNode &create_index, Stmt *&stmt)
{
  stmt = nullptr;

  const char *table_name = create_index.relation_name.c_str();
  const string &attribute_name =
      create_index.attribute_names.empty() ? create_index.attribute_name : create_index.attribute_names.front();
  if (is_blank(table_name) || is_blank(create_index.index_name.c_str()) || is_blank(attribute_name.c_str())) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, index name=%s, attribute name=%s",
        db, table_name, create_index.index_name.c_str(), attribute_name.c_str());
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  for (const string &name : create_index.attribute_names) {
    if (nullptr == table->table_meta().field(name.c_str())) {
      LOG_WARN("no such field in table. db=%s, table=%s, field name=%s", db->name(), table_name, name.c_str());
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }
  }

  vector<const FieldMeta *> field_metas;
  if (create_index.attribute_names.empty()) {
    field_metas.emplace_back(table->table_meta().field(attribute_name.c_str()));
  } else {
    field_metas.reserve(create_index.attribute_names.size());
    for (const string &name : create_index.attribute_names) {
      field_metas.emplace_back(table->table_meta().field(name.c_str()));
    }
  }

  const FieldMeta *field_meta = table->table_meta().field(attribute_name.c_str());
  if (nullptr == field_meta) {
    LOG_WARN("no such field in table. db=%s, table=%s, field name=%s", 
             db->name(), table_name, attribute_name.c_str());
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  if (table->index_name_exists(create_index.index_name.c_str())) {
    LOG_WARN("index with name(%s) already exists. table name=%s", create_index.index_name.c_str(), table_name);
    return RC::SCHEMA_INDEX_NAME_REPEAT;
  }

  if (create_index.unique && field_metas.size() > 1) {
    stmt = new CreateIndexStmt(table, field_metas, create_index.index_name);
  } else {
    stmt = new CreateIndexStmt(table, field_meta, create_index.index_name, create_index.unique);
  }
  return RC::SUCCESS;
}
