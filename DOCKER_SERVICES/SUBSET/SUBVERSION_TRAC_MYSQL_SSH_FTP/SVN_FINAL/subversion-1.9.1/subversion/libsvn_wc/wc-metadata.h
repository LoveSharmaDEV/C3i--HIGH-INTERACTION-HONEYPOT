/* This file is automatically generated from wc-metadata.sql and .dist_sandbox/subversion-1.9.1/subversion/libsvn_wc/token-map.h.
 * Do not edit this file -- edit the source and rerun gen-make.py */

#define STMT_CREATE_SCHEMA 0
#define STMT_0_INFO {"STMT_CREATE_SCHEMA", NULL}
#define STMT_0 \
  "CREATE TABLE REPOSITORY ( " \
  "  id INTEGER PRIMARY KEY AUTOINCREMENT, " \
  "  root  TEXT UNIQUE NOT NULL, " \
  "  uuid  TEXT NOT NULL " \
  "  ); " \
  "CREATE INDEX I_UUID ON REPOSITORY (uuid); " \
  "CREATE INDEX I_ROOT ON REPOSITORY (root); " \
  "CREATE TABLE WCROOT ( " \
  "  id  INTEGER PRIMARY KEY AUTOINCREMENT, " \
  "  local_abspath  TEXT UNIQUE " \
  "  ); " \
  "CREATE UNIQUE INDEX I_LOCAL_ABSPATH ON WCROOT (local_abspath); " \
  "CREATE TABLE PRISTINE ( " \
  "  checksum  TEXT NOT NULL PRIMARY KEY, " \
  "  compression  INTEGER, " \
  "  size  INTEGER NOT NULL, " \
  "  refcount  INTEGER NOT NULL, " \
  "  md5_checksum  TEXT NOT NULL " \
  "  ); " \
  "CREATE INDEX I_PRISTINE_MD5 ON PRISTINE (md5_checksum); " \
  "CREATE TABLE ACTUAL_NODE ( " \
  "  wc_id  INTEGER NOT NULL REFERENCES WCROOT (id), " \
  "  local_relpath  TEXT NOT NULL, " \
  "  parent_relpath  TEXT, " \
  "  properties  BLOB, " \
  "  conflict_old  TEXT, " \
  "  conflict_new  TEXT, " \
  "  conflict_working  TEXT, " \
  "  prop_reject  TEXT, " \
  "  changelist  TEXT, " \
  "  text_mod  TEXT, " \
  "  tree_conflict_data  TEXT, " \
  "  conflict_data  BLOB, " \
  "  older_checksum  TEXT REFERENCES PRISTINE (checksum), " \
  "  left_checksum  TEXT REFERENCES PRISTINE (checksum), " \
  "  right_checksum  TEXT REFERENCES PRISTINE (checksum), " \
  "  PRIMARY KEY (wc_id, local_relpath) " \
  "  ); " \
  "CREATE UNIQUE INDEX I_ACTUAL_PARENT ON ACTUAL_NODE (wc_id, parent_relpath, " \
  "                                                    local_relpath); " \
  "CREATE TABLE LOCK ( " \
  "  repos_id  INTEGER NOT NULL REFERENCES REPOSITORY (id), " \
  "  repos_relpath  TEXT NOT NULL, " \
  "  lock_token  TEXT NOT NULL, " \
  "  lock_owner  TEXT, " \
  "  lock_comment  TEXT, " \
  "  lock_date  INTEGER, " \
  "  PRIMARY KEY (repos_id, repos_relpath) " \
  "  ); " \
  "CREATE TABLE WORK_QUEUE ( " \
  "  id  INTEGER PRIMARY KEY AUTOINCREMENT, " \
  "  work  BLOB NOT NULL " \
  "  ); " \
  "CREATE TABLE WC_LOCK ( " \
  "  wc_id  INTEGER NOT NULL  REFERENCES WCROOT (id), " \
  "  local_dir_relpath  TEXT NOT NULL, " \
  "  locked_levels  INTEGER NOT NULL DEFAULT -1, " \
  "  PRIMARY KEY (wc_id, local_dir_relpath) " \
  " ); " \
  "PRAGMA user_version = " \
  APR_STRINGIFY(SVN_WC__VERSION) \
  "; " \
  ""

#define STMT_CREATE_NODES 1
#define STMT_1_INFO {"STMT_CREATE_NODES", NULL}
#define STMT_1 \
  "CREATE TABLE NODES ( " \
  "  wc_id  INTEGER NOT NULL REFERENCES WCROOT (id), " \
  "  local_relpath  TEXT NOT NULL, " \
  "  op_depth INTEGER NOT NULL, " \
  "  parent_relpath  TEXT, " \
  "  repos_id  INTEGER REFERENCES REPOSITORY (id), " \
  "  repos_path  TEXT, " \
  "  revision  INTEGER, " \
  "  presence  TEXT NOT NULL, " \
  "  moved_here  INTEGER, " \
  "  moved_to  TEXT, " \
  "  kind  TEXT NOT NULL, " \
  "  properties  BLOB, " \
  "  depth  TEXT, " \
  "  checksum  TEXT REFERENCES PRISTINE (checksum), " \
  "  symlink_target  TEXT, " \
  "  changed_revision  INTEGER, " \
  "  changed_date      INTEGER, " \
  "  changed_author    TEXT, " \
  "  translated_size  INTEGER, " \
  "  last_mod_time  INTEGER, " \
  "  dav_cache  BLOB, " \
  "  file_external  INTEGER, " \
  "  inherited_props  BLOB, " \
  "  PRIMARY KEY (wc_id, local_relpath, op_depth) " \
  "  ); " \
  "CREATE UNIQUE INDEX I_NODES_PARENT ON NODES (wc_id, parent_relpath, " \
  "                                             local_relpath, op_depth); " \
  "CREATE UNIQUE INDEX I_NODES_MOVED ON NODES (wc_id, moved_to, op_depth); " \
  "CREATE VIEW NODES_CURRENT AS " \
  "  SELECT * FROM nodes AS n " \
  "    WHERE op_depth = (SELECT MAX(op_depth) FROM nodes AS n2 " \
  "                      WHERE n2.wc_id = n.wc_id " \
  "                        AND n2.local_relpath = n.local_relpath); " \
  "CREATE VIEW NODES_BASE AS " \
  "  SELECT * FROM nodes " \
  "  WHERE op_depth = 0; " \
  ""

#define STMT_CREATE_NODES_TRIGGERS 2
#define STMT_2_INFO {"STMT_CREATE_NODES_TRIGGERS", NULL}
#define STMT_2 \
  "CREATE TRIGGER nodes_insert_trigger " \
  "AFTER INSERT ON nodes " \
  "WHEN NEW.checksum IS NOT NULL " \
  "BEGIN " \
  "  UPDATE pristine SET refcount = refcount + 1 " \
  "  WHERE checksum = NEW.checksum; " \
  "END; " \
  "CREATE TRIGGER nodes_delete_trigger " \
  "AFTER DELETE ON nodes " \
  "WHEN OLD.checksum IS NOT NULL " \
  "BEGIN " \
  "  UPDATE pristine SET refcount = refcount - 1 " \
  "  WHERE checksum = OLD.checksum; " \
  "END; " \
  "CREATE TRIGGER nodes_update_checksum_trigger " \
  "AFTER UPDATE OF checksum ON nodes " \
  "WHEN NEW.checksum IS NOT OLD.checksum " \
  "BEGIN " \
  "  UPDATE pristine SET refcount = refcount + 1 " \
  "  WHERE checksum = NEW.checksum; " \
  "  UPDATE pristine SET refcount = refcount - 1 " \
  "  WHERE checksum = OLD.checksum; " \
  "END; " \
  ""

#define STMT_CREATE_EXTERNALS 3
#define STMT_3_INFO {"STMT_CREATE_EXTERNALS", NULL}
#define STMT_3 \
  "CREATE TABLE EXTERNALS ( " \
  "  wc_id  INTEGER NOT NULL REFERENCES WCROOT (id), " \
  "  local_relpath  TEXT NOT NULL, " \
  "  parent_relpath  TEXT NOT NULL, " \
  "  repos_id  INTEGER NOT NULL REFERENCES REPOSITORY (id), " \
  "  presence  TEXT NOT NULL, " \
  "  kind  TEXT NOT NULL, " \
  "  def_local_relpath         TEXT NOT NULL, " \
  "  def_repos_relpath         TEXT NOT NULL, " \
  "  def_operational_revision  TEXT, " \
  "  def_revision              TEXT, " \
  "  PRIMARY KEY (wc_id, local_relpath) " \
  "); " \
  "CREATE UNIQUE INDEX I_EXTERNALS_DEFINED ON EXTERNALS (wc_id, " \
  "                                                      def_local_relpath, " \
  "                                                      local_relpath); " \
  ""

#define STMT_INSTALL_SCHEMA_STATISTICS 4
#define STMT_4_INFO {"STMT_INSTALL_SCHEMA_STATISTICS", NULL}
#define STMT_4 \
  "ANALYZE sqlite_master; " \
  "DELETE FROM sqlite_stat1 " \
  "WHERE tbl in ('NODES', 'ACTUAL_NODE', 'LOCK', 'WC_LOCK', 'EXTERNALS'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('NODES', 'sqlite_autoindex_NODES_1',               '8000 8000 2 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('NODES', 'I_NODES_PARENT',                         '8000 8000 10 2 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('NODES', 'I_NODES_MOVED',                          '8000 8000 1 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('ACTUAL_NODE', 'sqlite_autoindex_ACTUAL_NODE_1',   '8000 8000 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('ACTUAL_NODE', 'I_ACTUAL_PARENT',                  '8000 8000 10 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('LOCK', 'sqlite_autoindex_LOCK_1',                 '100 100 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('WC_LOCK', 'sqlite_autoindex_WC_LOCK_1',           '100 100 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('EXTERNALS','sqlite_autoindex_EXTERNALS_1',        '100 100 1'); " \
  "INSERT INTO sqlite_stat1(tbl, idx, stat) VALUES " \
  "    ('EXTERNALS','I_EXTERNALS_DEFINED',                 '100 100 3 1'); " \
  "ANALYZE sqlite_master; " \
  ""

#define STMT_UPGRADE_TO_20 5
#define STMT_5_INFO {"STMT_UPGRADE_TO_20", NULL}
#define STMT_5 \
  "UPDATE BASE_NODE SET checksum = (SELECT checksum FROM pristine " \
  "                                 WHERE md5_checksum = BASE_NODE.checksum) " \
  "WHERE EXISTS (SELECT 1 FROM pristine WHERE md5_checksum = BASE_NODE.checksum); " \
  "UPDATE WORKING_NODE SET checksum = (SELECT checksum FROM pristine " \
  "                                    WHERE md5_checksum = WORKING_NODE.checksum) " \
  "WHERE EXISTS (SELECT 1 FROM pristine " \
  "              WHERE md5_checksum = WORKING_NODE.checksum); " \
  "INSERT INTO NODES ( " \
  "       wc_id, local_relpath, op_depth, parent_relpath, " \
  "       repos_id, repos_path, revision, " \
  "       presence, depth, moved_here, moved_to, kind, " \
  "       changed_revision, changed_date, changed_author, " \
  "       checksum, properties, translated_size, last_mod_time, " \
  "       dav_cache, symlink_target, file_external ) " \
  "SELECT wc_id, local_relpath, 0 , parent_relpath, " \
  "       repos_id, repos_relpath, revnum, " \
  "       presence, depth, NULL , NULL , kind, " \
  "       changed_rev, changed_date, changed_author, " \
  "       checksum, properties, translated_size, last_mod_time, " \
  "       dav_cache, symlink_target, file_external " \
  "FROM BASE_NODE; " \
  "INSERT INTO NODES ( " \
  "       wc_id, local_relpath, op_depth, parent_relpath, " \
  "       repos_id, repos_path, revision, " \
  "       presence, depth, moved_here, moved_to, kind, " \
  "       changed_revision, changed_date, changed_author, " \
  "       checksum, properties, translated_size, last_mod_time, " \
  "       dav_cache, symlink_target, file_external ) " \
  "SELECT wc_id, local_relpath, 2 , parent_relpath, " \
  "       copyfrom_repos_id, copyfrom_repos_path, copyfrom_revnum, " \
  "       presence, depth, NULL , NULL , kind, " \
  "       changed_rev, changed_date, changed_author, " \
  "       checksum, properties, translated_size, last_mod_time, " \
  "       NULL , symlink_target, NULL " \
  "FROM WORKING_NODE; " \
  "DROP TABLE BASE_NODE; " \
  "DROP TABLE WORKING_NODE; " \
  "PRAGMA user_version = 20; " \
  ""

#define STMT_UPGRADE_TO_21 6
#define STMT_6_INFO {"STMT_UPGRADE_TO_21", NULL}
#define STMT_6 \
  "PRAGMA user_version = 21; " \
  ""

#define STMT_UPGRADE_21_SELECT_OLD_TREE_CONFLICT 7
#define STMT_7_INFO {"STMT_UPGRADE_21_SELECT_OLD_TREE_CONFLICT", NULL}
#define STMT_7 \
  "SELECT wc_id, local_relpath, tree_conflict_data " \
  "FROM actual_node " \
  "WHERE tree_conflict_data IS NOT NULL " \
  ""

#define STMT_UPGRADE_21_ERASE_OLD_CONFLICTS 8
#define STMT_8_INFO {"STMT_UPGRADE_21_ERASE_OLD_CONFLICTS", NULL}
#define STMT_8 \
  "UPDATE actual_node SET tree_conflict_data = NULL " \
  ""

#define STMT_UPGRADE_TO_22 9
#define STMT_9_INFO {"STMT_UPGRADE_TO_22", NULL}
#define STMT_9 \
  "UPDATE actual_node SET tree_conflict_data = conflict_data; " \
  "UPDATE actual_node SET conflict_data = NULL; " \
  "PRAGMA user_version = 22; " \
  ""

#define STMT_UPGRADE_TO_23 10
#define STMT_10_INFO {"STMT_UPGRADE_TO_23", NULL}
#define STMT_10 \
  "PRAGMA user_version = 23; " \
  ""

#define STMT_UPGRADE_23_HAS_WORKING_NODES 11
#define STMT_11_INFO {"STMT_UPGRADE_23_HAS_WORKING_NODES", NULL}
#define STMT_11 \
  "SELECT 1 FROM nodes WHERE op_depth > 0 " \
  "LIMIT 1 " \
  ""

#define STMT_UPGRADE_TO_24 12
#define STMT_12_INFO {"STMT_UPGRADE_TO_24", NULL}
#define STMT_12 \
  "UPDATE pristine SET refcount = " \
  "  (SELECT COUNT(*) FROM nodes " \
  "   WHERE checksum = pristine.checksum ); " \
  "PRAGMA user_version = 24; " \
  ""

#define STMT_UPGRADE_TO_25 13
#define STMT_13_INFO {"STMT_UPGRADE_TO_25", NULL}
#define STMT_13 \
  "DROP VIEW IF EXISTS NODES_CURRENT; " \
  "CREATE VIEW NODES_CURRENT AS " \
  "  SELECT * FROM nodes " \
  "    JOIN (SELECT wc_id, local_relpath, MAX(op_depth) AS op_depth FROM nodes " \
  "          GROUP BY wc_id, local_relpath) AS filter " \
  "    ON nodes.wc_id = filter.wc_id " \
  "      AND nodes.local_relpath = filter.local_relpath " \
  "      AND nodes.op_depth = filter.op_depth; " \
  "PRAGMA user_version = 25; " \
  ""

#define STMT_UPGRADE_TO_26 14
#define STMT_14_INFO {"STMT_UPGRADE_TO_26", NULL}
#define STMT_14 \
  "DROP VIEW IF EXISTS NODES_BASE; " \
  "CREATE VIEW NODES_BASE AS " \
  "  SELECT * FROM nodes " \
  "  WHERE op_depth = 0; " \
  "PRAGMA user_version = 26; " \
  ""

#define STMT_UPGRADE_TO_27 15
#define STMT_15_INFO {"STMT_UPGRADE_TO_27", NULL}
#define STMT_15 \
  "PRAGMA user_version = 27; " \
  ""

#define STMT_UPGRADE_27_HAS_ACTUAL_NODES_CONFLICTS 16
#define STMT_16_INFO {"STMT_UPGRADE_27_HAS_ACTUAL_NODES_CONFLICTS", NULL}
#define STMT_16 \
  "SELECT 1 FROM actual_node " \
  "WHERE NOT ((prop_reject IS NULL) AND (conflict_old IS NULL) " \
  "           AND (conflict_new IS NULL) AND (conflict_working IS NULL) " \
  "           AND (tree_conflict_data IS NULL)) " \
  "LIMIT 1 " \
  ""

#define STMT_UPGRADE_TO_28 17
#define STMT_17_INFO {"STMT_UPGRADE_TO_28", NULL}
#define STMT_17 \
  "UPDATE NODES SET checksum = (SELECT checksum FROM pristine " \
  "                             WHERE md5_checksum = nodes.checksum) " \
  "WHERE EXISTS (SELECT 1 FROM pristine WHERE md5_checksum = nodes.checksum); " \
  "PRAGMA user_version = 28; " \
  ""

#define STMT_UPGRADE_TO_29 18
#define STMT_18_INFO {"STMT_UPGRADE_TO_29", NULL}
#define STMT_18 \
  "DROP TRIGGER IF EXISTS nodes_update_checksum_trigger; " \
  "DROP TRIGGER IF EXISTS nodes_insert_trigger; " \
  "DROP TRIGGER IF EXISTS nodes_delete_trigger; " \
  "CREATE TRIGGER nodes_update_checksum_trigger " \
  "AFTER UPDATE OF checksum ON nodes " \
  "WHEN NEW.checksum IS NOT OLD.checksum " \
  "BEGIN " \
  "  UPDATE pristine SET refcount = refcount + 1 " \
  "  WHERE checksum = NEW.checksum; " \
  "  UPDATE pristine SET refcount = refcount - 1 " \
  "  WHERE checksum = OLD.checksum; " \
  "END; " \
  "CREATE TRIGGER nodes_insert_trigger " \
  "AFTER INSERT ON nodes " \
  "WHEN NEW.checksum IS NOT NULL " \
  "BEGIN " \
  "  UPDATE pristine SET refcount = refcount + 1 " \
  "  WHERE checksum = NEW.checksum; " \
  "END; " \
  "CREATE TRIGGER nodes_delete_trigger " \
  "AFTER DELETE ON nodes " \
  "WHEN OLD.checksum IS NOT NULL " \
  "BEGIN " \
  "  UPDATE pristine SET refcount = refcount - 1 " \
  "  WHERE checksum = OLD.checksum; " \
  "END; " \
  "PRAGMA user_version = 29; " \
  ""

#define STMT_UPGRADE_TO_30 19
#define STMT_19_INFO {"STMT_UPGRADE_TO_30", NULL}
#define STMT_19 \
  "CREATE UNIQUE INDEX IF NOT EXISTS I_NODES_MOVED " \
  "ON NODES (wc_id, moved_to, op_depth); " \
  "CREATE INDEX IF NOT EXISTS I_PRISTINE_MD5 ON PRISTINE (md5_checksum); " \
  "UPDATE nodes SET presence = \"server-excluded\" WHERE presence = \"absent\"; " \
  "UPDATE nodes SET file_external=1 WHERE file_external IS NOT NULL; " \
  ""

#define STMT_UPGRADE_30_SELECT_CONFLICT_SEPARATE 20
#define STMT_20_INFO {"STMT_UPGRADE_30_SELECT_CONFLICT_SEPARATE", NULL}
#define STMT_20 \
  "SELECT wc_id, local_relpath, " \
  "  conflict_old, conflict_working, conflict_new, prop_reject, tree_conflict_data " \
  "FROM actual_node " \
  "WHERE conflict_old IS NOT NULL " \
  "   OR conflict_working IS NOT NULL " \
  "   OR conflict_new IS NOT NULL " \
  "   OR prop_reject IS NOT NULL " \
  "   OR tree_conflict_data IS NOT NULL " \
  "ORDER by wc_id, local_relpath " \
  ""

#define STMT_UPGRADE_30_SET_CONFLICT 21
#define STMT_21_INFO {"STMT_UPGRADE_30_SET_CONFLICT", NULL}
#define STMT_21 \
  "UPDATE actual_node SET conflict_data = ?3, conflict_old = NULL, " \
  "  conflict_working = NULL, conflict_new = NULL, prop_reject = NULL, " \
  "  tree_conflict_data = NULL " \
  "WHERE wc_id = ?1 and local_relpath = ?2 " \
  ""

#define STMT_UPGRADE_TO_31_ALTER_TABLE 22
#define STMT_22_INFO {"STMT_UPGRADE_TO_31_ALTER_TABLE", NULL}
#define STMT_22 \
  "ALTER TABLE NODES ADD COLUMN inherited_props BLOB; " \
  ""

#define STMT_UPGRADE_TO_31_FINALIZE 23
#define STMT_23_INFO {"STMT_UPGRADE_TO_31_FINALIZE", NULL}
#define STMT_23 \
  "DROP INDEX IF EXISTS I_ACTUAL_CHANGELIST; " \
  "DROP INDEX IF EXISTS I_EXTERNALS_PARENT; " \
  "DROP INDEX I_NODES_PARENT; " \
  "CREATE UNIQUE INDEX I_NODES_PARENT ON NODES (wc_id, parent_relpath, " \
  "                                             local_relpath, op_depth); " \
  "DROP INDEX I_ACTUAL_PARENT; " \
  "CREATE UNIQUE INDEX I_ACTUAL_PARENT ON ACTUAL_NODE (wc_id, parent_relpath, " \
  "                                                    local_relpath); " \
  "PRAGMA user_version = 31; " \
  ""

#define STMT_UPGRADE_31_SELECT_WCROOT_NODES 24
#define STMT_24_INFO {"STMT_UPGRADE_31_SELECT_WCROOT_NODES", NULL}
#define STMT_24 \
  "SELECT l.wc_id, l.local_relpath FROM nodes as l " \
  "LEFT OUTER JOIN nodes as r " \
  "ON l.wc_id = r.wc_id " \
  "   AND r.local_relpath = l.parent_relpath " \
  "   AND r.op_depth = 0 " \
  "WHERE l.op_depth = 0 " \
  "  AND l.repos_path != '' " \
  "  AND ((l.repos_id IS NOT r.repos_id) " \
  "       OR (l.repos_path IS NOT (CASE WHEN (r.local_relpath) = '' THEN (CASE WHEN (r.repos_path) = '' THEN (l.local_relpath) WHEN (l.local_relpath) = '' THEN (r.repos_path) ELSE (r.repos_path) || '/' || (l.local_relpath) END) WHEN (r.repos_path) = '' THEN (CASE WHEN (r.local_relpath) = '' THEN (l.local_relpath)  WHEN SUBSTR((l.local_relpath), 1, LENGTH(r.local_relpath)) = (r.local_relpath)  THEN CASE WHEN LENGTH(r.local_relpath) = LENGTH(l.local_relpath) THEN '' WHEN SUBSTR((l.local_relpath), LENGTH(r.local_relpath)+1, 1) = '/' THEN SUBSTR((l.local_relpath), LENGTH(r.local_relpath)+2) END END) WHEN SUBSTR((l.local_relpath), 1, LENGTH(r.local_relpath)) = (r.local_relpath) THEN CASE WHEN LENGTH(r.local_relpath) = LENGTH(l.local_relpath) THEN (r.repos_path) WHEN SUBSTR((l.local_relpath), LENGTH(r.local_relpath)+1, 1) = '/' THEN (r.repos_path) || SUBSTR((l.local_relpath), LENGTH(r.local_relpath)+1) END END))) " \
  ""

#define STMT_UPGRADE_TO_32 25
#define STMT_25_INFO {"STMT_UPGRADE_TO_32", NULL}
#define STMT_25 \
  "DROP INDEX IF EXISTS I_ACTUAL_CHANGELIST; " \
  "DROP INDEX IF EXISTS I_EXTERNALS_PARENT; " \
  "CREATE INDEX I_EXTERNALS_PARENT ON EXTERNALS (wc_id, parent_relpath); " \
  "DROP INDEX I_NODES_PARENT; " \
  "CREATE UNIQUE INDEX I_NODES_PARENT ON NODES (wc_id, parent_relpath, " \
  "                                             local_relpath, op_depth); " \
  "DROP INDEX I_ACTUAL_PARENT; " \
  "CREATE UNIQUE INDEX I_ACTUAL_PARENT ON ACTUAL_NODE (wc_id, parent_relpath, " \
  "                                                    local_relpath); " \
  "-- format: YYY " \
  ""

#define WC_METADATA_SQL_99 \
  "CREATE TABLE ACTUAL_NODE_BACKUP ( " \
  "  wc_id  INTEGER NOT NULL, " \
  "  local_relpath  TEXT NOT NULL, " \
  "  parent_relpath  TEXT, " \
  "  properties  BLOB, " \
  "  conflict_old  TEXT, " \
  "  conflict_new  TEXT, " \
  "  conflict_working  TEXT, " \
  "  prop_reject  TEXT, " \
  "  changelist  TEXT, " \
  "  text_mod  TEXT " \
  "  ); " \
  "INSERT INTO ACTUAL_NODE_BACKUP SELECT " \
  "  wc_id, local_relpath, parent_relpath, properties, conflict_old, " \
  "  conflict_new, conflict_working, prop_reject, changelist, text_mod " \
  "FROM ACTUAL_NODE; " \
  "DROP TABLE ACTUAL_NODE; " \
  "CREATE TABLE ACTUAL_NODE ( " \
  "  wc_id  INTEGER NOT NULL REFERENCES WCROOT (id), " \
  "  local_relpath  TEXT NOT NULL, " \
  "  parent_relpath  TEXT, " \
  "  properties  BLOB, " \
  "  conflict_old  TEXT, " \
  "  conflict_new  TEXT, " \
  "  conflict_working  TEXT, " \
  "  prop_reject  TEXT, " \
  "  changelist  TEXT, " \
  "  text_mod  TEXT, " \
  "  PRIMARY KEY (wc_id, local_relpath) " \
  "  ); " \
  "CREATE UNIQUE INDEX I_ACTUAL_PARENT ON ACTUAL_NODE (wc_id, parent_relpath, " \
  "                                                    local_relpath); " \
  "INSERT INTO ACTUAL_NODE SELECT " \
  "  wc_id, local_relpath, parent_relpath, properties, conflict_old, " \
  "  conflict_new, conflict_working, prop_reject, changelist, text_mod " \
  "FROM ACTUAL_NODE_BACKUP; " \
  "DROP TABLE ACTUAL_NODE_BACKUP; " \
  ""

#define WC_METADATA_SQL_DECLARE_STATEMENTS(varname) \
  static const char * const varname[] = { \
    STMT_0, \
    STMT_1, \
    STMT_2, \
    STMT_3, \
    STMT_4, \
    STMT_5, \
    STMT_6, \
    STMT_7, \
    STMT_8, \
    STMT_9, \
    STMT_10, \
    STMT_11, \
    STMT_12, \
    STMT_13, \
    STMT_14, \
    STMT_15, \
    STMT_16, \
    STMT_17, \
    STMT_18, \
    STMT_19, \
    STMT_20, \
    STMT_21, \
    STMT_22, \
    STMT_23, \
    STMT_24, \
    STMT_25, \
    NULL \
  }

#define WC_METADATA_SQL_DECLARE_STATEMENT_INFO(varname) \
  static const char * const varname[][2] = { \
    STMT_0_INFO, \
    STMT_1_INFO, \
    STMT_2_INFO, \
    STMT_3_INFO, \
    STMT_4_INFO, \
    STMT_5_INFO, \
    STMT_6_INFO, \
    STMT_7_INFO, \
    STMT_8_INFO, \
    STMT_9_INFO, \
    STMT_10_INFO, \
    STMT_11_INFO, \
    STMT_12_INFO, \
    STMT_13_INFO, \
    STMT_14_INFO, \
    STMT_15_INFO, \
    STMT_16_INFO, \
    STMT_17_INFO, \
    STMT_18_INFO, \
    STMT_19_INFO, \
    STMT_20_INFO, \
    STMT_21_INFO, \
    STMT_22_INFO, \
    STMT_23_INFO, \
    STMT_24_INFO, \
    STMT_25_INFO, \
    {NULL, NULL} \
  }
