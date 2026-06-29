/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/executor/show_index_executor.h"

#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/executor/sql_result.h"
#include "sql/operator/string_list_physical_operator.h"
#include "sql/stmt/show_index_stmt.h"
#include "storage/table/table.h"

RC ShowIndexExecutor::execute(SQLStageEvent *sql_event)
{
  auto      *stmt       = static_cast<ShowIndexStmt *>(sql_event->stmt());
  SqlResult *sql_result = sql_event->session_event()->sql_result();
  Table     *table      = stmt->table();

  TupleSchema tuple_schema;
  tuple_schema.append_cell(TupleCellSpec("", "Table", "Table"));
  tuple_schema.append_cell(TupleCellSpec("", "Non_unique", "Non_unique"));
  tuple_schema.append_cell(TupleCellSpec("", "Key_name", "Key_name"));
  tuple_schema.append_cell(TupleCellSpec("", "Seq_in_index", "Seq_in_index"));
  tuple_schema.append_cell(TupleCellSpec("", "Column_name", "Column_name"));
  sql_result->set_tuple_schema(tuple_schema);

  auto             *oper       = new StringListPhysicalOperator;
  const TableMeta &table_meta = table->table_meta();
  for (int i = 0; i < table_meta.index_num(); i++) {
    const IndexMeta *index_meta = table_meta.index(i);
    oper->append({table->name(), index_meta->is_unique() ? "0" : "1", index_meta->name(), "1", index_meta->field()});
  }

  sql_result->set_operator(std::unique_ptr<PhysicalOperator>(oper));
  return RC::SUCCESS;
}
