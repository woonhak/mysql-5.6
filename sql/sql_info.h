/* Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.

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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#ifndef SQL_INFO_H
#define SQL_INFO_H

#include <string>
#include <vector>

#include "sql/sql_error.h"
#include "storage/perfschema/table_sql_findings.h"

/*
  Possible values used for variables to control collection of MySQL stats
  - sql_findings_control,
  - sql_plans_control,
  - column_stats_control
  Values
  - OFF_HARD: stop the collection and all data in the corresponding
              in-memory structures is evicted
  - OFF_SOFT: stop collecting the stats but keep the data collected so far
  - ON:       (re-)start the collection

  Keep the enum in the sync with sql_info_control_values[] (sys_vars.cc)
*/
enum enum_sql_info_control {
  SQL_INFO_CONTROL_OFF_HARD = 0,
  SQL_INFO_CONTROL_OFF_SOFT = 1,
  SQL_INFO_CONTROL_ON = 2,
  /* Add new control before the following line */
  SQL_INFO_CONTROL_INVALID
};

/***********************************************************************
              Begin - Functions to support SQL findings
************************************************************************/

/* SQL Finding - stores information about one SQL finding */
typedef struct st_sql_finding {
  uint code;                                /* error code */
  Sql_condition::enum_severity_level level; /* warning level */
  std::string message;                      /* message */
  std::string query_text;                   /* query text */
  ulonglong count;         /* number of times the finding was recorded */
  ulonglong last_recorded; /* last recorded, seconds since epoch */
} SQL_FINDING;

/* SQL_FINDING_VEC - stores all the findings for a SQL statement.
   The lookup key is the code/error number
 */
typedef std::vector<SQL_FINDING> SQL_FINDING_VEC;

void free_global_sql_findings();
void store_sql_findings(THD *thd, char *query_text);
std::vector<sql_findings_row> get_all_sql_findings();

/***********************************************************************
                End - Functions to support SQL findings
************************************************************************/

#endif
