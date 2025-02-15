import os
import unittest
from .helpers.ptrack_helpers import ProbackupTest, ProbackupException

class CVE_2018_1058(ProbackupTest, unittest.TestCase):

    # @unittest.skip("skip")
    def test_basic_default_search_path(self):
        """"""
        backup_dir = os.path.join(self.tmp_path, self.module_name, self.fname, 'backup')
        node = self.make_simple_node(
            base_dir=os.path.join(self.module_name, self.fname, 'node'),
            set_replication=True)

        self.init_pb(backup_dir)
        self.add_instance(backup_dir, 'node', node)
        node.slow_start()

        node.safe_psql(
            'postgres',
            "CREATE FUNCTION public.pgpro_edition() "
            "RETURNS text "
            "AS $$ "
            "BEGIN "
            "  RAISE 'pg_backup vulnerable!'; "
            "END "
            "$$ LANGUAGE plpgsql")

        self.backup_node(backup_dir, 'node', node, backup_type='full', options=['--stream'])

    # @unittest.skip("skip")
    def test_basic_backup_modified_search_path(self):
        """"""
        backup_dir = os.path.join(self.tmp_path, self.module_name, self.fname, 'backup')
        node = self.make_simple_node(
            base_dir=os.path.join(self.module_name, self.fname, 'node'),
            set_replication=True)
        self.set_auto_conf(node, options={'search_path': 'public,pg_catalog'})

        self.init_pb(backup_dir)
        self.add_instance(backup_dir, 'node', node)
        node.slow_start()

        node.safe_psql(
            'postgres',
            "CREATE FUNCTION public.pg_control_checkpoint(OUT timeline_id integer, OUT dummy integer) "
            "RETURNS record "
            "AS $$ "
            "BEGIN "
            "  RAISE '% vulnerable!', 'pg_backup'; "
            "END "
            "$$ LANGUAGE plpgsql")

        node.safe_psql(
            'postgres',
            "CREATE FUNCTION public.pg_proc(OUT proname name, OUT dummy integer) "
            "RETURNS record "
            "AS $$ "
            "BEGIN "
            "  RAISE '% vulnerable!', 'pg_backup'; "
            "END "
            "$$ LANGUAGE plpgsql; "
            "CREATE VIEW public.pg_proc AS SELECT proname FROM public.pg_proc()")

        self.backup_node(backup_dir, 'node', node, backup_type='full', options=['--stream'])

        log_file = os.path.join(node.logs_dir, 'postgresql.log')
        with open(log_file, 'r') as f:
            log_content = f.read()
            self.assertFalse(
                'pg_backup vulnerable!' in log_content)

    # @unittest.skip("skip")
    def test_basic_checkdb_modified_search_path(self):
        """"""
        node = self.make_simple_node(
            base_dir=os.path.join(self.module_name, self.fname, 'node'),
            initdb_params=['--data-checksums'])
        self.set_auto_conf(node, options={'search_path': 'public,pg_catalog'})
        node.slow_start()

        node.safe_psql(
            'postgres',
            "CREATE FUNCTION public.pg_database(OUT datname name, OUT oid oid, OUT dattablespace oid) "
            "RETURNS record "
            "AS $$ "
            "BEGIN "
            "  RAISE 'pg_backup vulnerable!'; "
            "END "
            "$$ LANGUAGE plpgsql; "
            "CREATE VIEW public.pg_database AS SELECT * FROM public.pg_database()")
        
        node.safe_psql(
            'postgres',
            "CREATE FUNCTION public.pg_extension(OUT extname name, OUT extnamespace oid, OUT extversion text) "
            "RETURNS record "
            "AS $$ "
            "BEGIN "
            "  RAISE 'pg_backup vulnerable!'; "
            "END "
            "$$ LANGUAGE plpgsql; "
            "CREATE FUNCTION public.pg_namespace(OUT oid oid, OUT nspname name) "
            "RETURNS record "
            "AS $$ "
            "BEGIN "
            "  RAISE 'pg_backup vulnerable!'; "
            "END "
            "$$ LANGUAGE plpgsql; "
            "CREATE VIEW public.pg_extension AS SELECT * FROM public.pg_extension();"
            "CREATE VIEW public.pg_namespace AS SELECT * FROM public.pg_namespace();"
            )

        try:
            self.checkdb_node(
                options=[
                    '--amcheck',
                    '--skip-block-validation',
                    '-d', 'postgres', '-p', str(node.port)])
            self.assertEqual(
                1, 0,
                "Expecting Error because amcheck{,_next} not installed\n"
                " Output: {0} \n CMD: {1}".format(
                    repr(self.output), self.cmd))
        except ProbackupException as e:
            self.assertIn(
                "WARNING: Extension 'amcheck' or 'amcheck_next' are not installed in database postgres",
                e.message,
                "\n Unexpected Error Message: {0}\n CMD: {1}".format(
                    repr(e.message), self.cmd))
