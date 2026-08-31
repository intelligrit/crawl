"""Tests for the user database backends.

The SQLite suite (`UserDBTest`, defined next to the code in userdb.py) always
runs. The PostgreSQL suite reruns the same tests against a live server and is
skipped unless `WEBTILES_TEST_POSTGRESQL_DSN` is set, e.g.

    WEBTILES_TEST_POSTGRESQL_DSN=postgresql://user:pw@localhost/webtiles pytest

It drops and recreates the webtiles tables in that database.
"""
import os
import unittest

from webtiles import config, userdb
from webtiles.userdb import UserDBTest  # noqa: F401 -- collected by pytest

PG_DSN = os.environ.get("WEBTILES_TEST_POSTGRESQL_DSN")


class PostgresSQLTranslationTest(unittest.TestCase):
    """The SQLite dialect used throughout userdb.py is rewritten for
    PostgreSQL at execute time; these pin the rewrite rules."""

    def test_placeholders(self):
        self.assertEqual(userdb.pg_sql("SELECT 1 FROM t WHERE a=? AND b=?"),
                         "SELECT 1 FROM t WHERE a=%s AND b=%s")

    def test_collations_are_dropped(self):
        # username/email/token columns are citext on PostgreSQL, so the
        # SQLite collation clauses have no equivalent and must vanish.
        self.assertEqual(userdb.pg_sql("WHERE username=?\n        COLLATE NOCASE"),
                         "WHERE username=%s")
        self.assertEqual(userdb.pg_sql("WHERE t.token = ? COLLATE RTRIM"),
                         "WHERE t.token = %s")

    def test_datetime_functions(self):
        self.assertEqual(userdb.pg_sql("VALUES (?,datetime('now'),?)"),
                         "VALUES (%s,now(),%s)")
        self.assertEqual(
            userdb.pg_sql("t.token_time > datetime('now','-12 hours')"),
            "t.token_time > (now() - interval '12 hours')")
        self.assertEqual(
            userdb.pg_sql("SET token_time=datetime('now', '-2 hours')"),
            "SET token_time=(now() - interval '2 hours')")

    def test_upsert(self):
        sql = userdb.pg_sql("INSERT OR REPLACE INTO mutesettings (username, mutelist) "
                            "VALUES (?,?);")
        self.assertEqual(sql, "INSERT INTO mutesettings (username, mutelist) VALUES (%s,%s) "
                              "ON CONFLICT (username) DO UPDATE SET mutelist = EXCLUDED.mutelist;")


@unittest.skipUnless(PG_DSN, "WEBTILES_TEST_POSTGRESQL_DSN not set")
class PostgresUserDBTest(UserDBTest):
    """Exactly the SQLite test suite, against PostgreSQL."""

    def setUp(self):
        if not self.logging_init:
            import webtiles.server as server
            server.init_logging(config.get('logging_config'))
            self.logging_init = True
        self.config_shim = dict(
            userdb_backend="postgresql",
            userdb_dsn=PG_DSN,
            # unused on this backend; set so tearDown of the base class is inert
            settings_db="./unittest_pg_unused_settings.db3",
            password_db="./unittest_pg_unused_passwd.db3",
            dgl_mode=True)
        config.server_config = self.config_shim
        self._drop_tables()
        userdb.init_db_connections(quiet=True)

    def tearDown(self):
        self._drop_tables()
        userdb.user_db.close()
        userdb.settings_db.close()
        super().tearDown()

    def _drop_tables(self):
        db = userdb.crawl_pg_db(PG_DSN)
        try:
            with db:
                db.execute("DROP TABLE IF EXISTS recovery_tokens, mutesettings, dglusers")
        finally:
            db.close()

    def test_backend_selected(self):
        self.assertIsInstance(userdb.user_db, userdb.crawl_pg_db)
        self.assertIsInstance(userdb.settings_db, userdb.crawl_pg_db)

    def test_case_insensitive_username_is_unique(self):
        self.assertIsNone(userdb.register_user("Test", "hunter2", "a@example.com"))
        self.assertEqual(userdb.register_user("tEsT", "hunter2", "b@example.com"),
                         "User already exists!")
        self.assertEqual(userdb.get_user_info("TEST").username, "Test")

    def test_blocklist_upsert(self):
        userdb.set_blocklist("test", "alice")
        userdb.set_blocklist("TEST", "alice bob")
        self.assertEqual(userdb.get_blocklist("test"), "alice bob")
