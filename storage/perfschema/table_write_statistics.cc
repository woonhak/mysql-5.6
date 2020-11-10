/* Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  This program is also distributed with certain software (including
  but not limited to OpenSSL) that is licensed under separate terms,
  as designated in a particular file or component or in included license
  documentation.  The authors of MySQL hereby grant you an additional
  permission to link the program and your derivative works with the
  separately licensed software that they have included with MySQL.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License, version 2.0, for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
  */

/**
  @file storage/perfschema/table_write_statistics.h
  Performance schema write_statistics table.
*/

#include "storage/perfschema/table_write_statistics.h"

#include "sql/field.h"
#include "sql/plugin_table.h"
#include "sql/rpl_lag_manager.h"
#include "sql/sql_class.h"
#include "sql/table.h"
#include "storage/perfschema/pfs_buffer_container.h"

#include "storage/perfschema/table_helper.h"

THR_LOCK table_write_statistics::m_table_lock;
std::atomic<int> table_write_statistics::m_most_recent_size(0);

Plugin_table table_write_statistics::m_table_def(
    /* Schema name */
    "performance_schema",
    /* Name */
    "write_statistics",
    /* Definition */
    "  TIMESTAMP TIMESTAMP NOT NULL default 0,\n"
    "  TYPE VARCHAR(16),\n"
    "  VALUE VARCHAR(64),\n"
    "  WRITE_DATA_BYTES BIGINT unsigned NOT NULL,\n"
    "  CPU_WRITE_TIME_MS BIGINT unsigned NOT NULL\n",
    /* Options */
    " ENGINE=PERFORMANCE_SCHEMA",
    /* Tablespace */
    nullptr);

PFS_engine_table_share table_write_statistics::m_share = {
    &pfs_readonly_acl,
    table_write_statistics::create,
    NULL, /* write_row */
    NULL, /* delete_all_rows */
    table_write_statistics::get_row_count,
    sizeof(PFS_simple_index),
    &table_write_statistics::m_table_lock,
    &table_write_statistics::m_table_def,
    false, /* perpetual */
    PFS_engine_table_proxy(),
    {0},
    false /* m_in_purgatory */
};

enum write_statistics_field_offset {
  FO_TIMESTAMP,
  FO_TYPE,
  FO_VALUE,
  FO_WRITE_DATA_BYTES,
  FO_CPU_WRITE_TIME_MS
};

table_write_statistics::table_write_statistics()
    : PFS_engine_table(&m_share, &m_pos), m_pos(0) {
  m_all_rows = get_all_write_statistics();
  table_write_statistics::m_most_recent_size = m_all_rows.size();
  m_current_row = nullptr;
}

PFS_engine_table *table_write_statistics::create(PFS_engine_table_share *) {
  return new table_write_statistics();
}

ha_rows table_write_statistics::get_row_count(void) {
  /*
    To hint the optimizer we return the most recent size of
    the table when we last loaded the stats
  */
  return m_most_recent_size;
}

void table_write_statistics::reset_position(void) { m_pos.set_at(0u); }

int table_write_statistics::rnd_next(void) {
  if (m_pos.m_index >= m_all_rows.size()) {
    m_current_row = nullptr;
    return HA_ERR_END_OF_FILE;
  }
  m_current_row = &m_all_rows[m_pos.m_index];
  m_pos.next();
  return 0;
}

int table_write_statistics::rnd_pos(const void *pos) {
  set_position(pos);
  if (m_pos.m_index >= m_all_rows.size()) {
    m_current_row = nullptr;
    return HA_ERR_RECORD_DELETED;
  }
  m_current_row = &m_all_rows[m_pos.m_index];
  return 0;
}

int table_write_statistics::read_row_values(TABLE *table, unsigned char *buf,
                                            Field **fields, bool read_all) {
  Field *f;
  const auto &curr_row = *m_current_row;

  /* Set the null bits */
  DBUG_ASSERT(table->s->null_bytes == 1);
  buf[0] = 0;

  for (; (f = *fields); fields++) {
    if (read_all || bitmap_is_set(table->read_set, f->field_index)) {
      switch (f->field_index) {
        case FO_TIMESTAMP:
          // convert to micro time
          set_field_timestamp(f, curr_row.timestamp() * 1000000);
          break;
        case FO_TYPE:
          set_field_varchar_utf8(f, curr_row.type().c_str(),
                                 curr_row.type().length());
          break;
        case FO_VALUE:
          set_field_varchar_utf8(f, curr_row.value().c_str(),
                                 curr_row.value().length());
          break;
        case FO_WRITE_DATA_BYTES:
          set_field_ulonglong(f, curr_row.write_data_bytes());
          break;
        case FO_CPU_WRITE_TIME_MS:
          set_field_ulonglong(f, curr_row.cpu_write_time_ms());
          break;
        default:
          DBUG_ASSERT(false);
      }
    }
  }

  return 0;
}
