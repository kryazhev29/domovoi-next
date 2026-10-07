"""Проверки изоляции пула, строгого результата и заблокированной уборки TEMP."""
import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import tempfile
import threading
import unittest
from unittest.mock import patch

from run_all import run_groups
from temp_cleanup import TemporaryDirectory


class CleanupTests(unittest.TestCase):
    @unittest.skipUnless(os.name == 'nt', 'Блокировка удаления файла Windows')
    def test_real_windows_lock(self):
        with tempfile.TemporaryDirectory() as parent:
            directory = TemporaryDirectory(dir=parent)
            executable = Path(directory.name)/'test.exe'
            handle = executable.open('wb')
            timer = threading.Timer(0.25, handle.close)
            try:
                with self.assertRaises(PermissionError):
                    executable.unlink()
                timer.start()
                directory.cleanup()
                self.assertFalse(Path(directory.name).exists())
            finally:
                handle.close()
                timer.cancel()
                if timer.ident is not None:
                    timer.join()

    def test_retries_permission_error(self):
        with tempfile.TemporaryDirectory() as parent:
            directory = TemporaryDirectory(dir=parent)
            original = tempfile.TemporaryDirectory._rmtree
            calls = []
            def locked(name, **kwargs):
                calls.append(name)
                if len(calls) < 3:
                    raise PermissionError('занят test.exe')
                return original(name, **kwargs)
            with patch.object(tempfile.TemporaryDirectory, '_rmtree', side_effect=locked), patch('temp_cleanup.time.sleep'):
                directory.cleanup()
            self.assertEqual(len(calls), 3)
            self.assertFalse(Path(directory.name).exists())

    def test_persistent_lock_preserves_test_exception(self):
        with tempfile.TemporaryDirectory() as parent:
            log = io.StringIO()
            with contextlib.redirect_stderr(log), patch.object(tempfile.TemporaryDirectory, '_rmtree', side_effect=PermissionError('занят')), patch('temp_cleanup.time.sleep'):
                with self.assertRaisesRegex(RuntimeError, 'ошибка проверки'):
                    with TemporaryDirectory(dir=parent) as folder:
                        raise RuntimeError('ошибка проверки')
            self.assertTrue(Path(folder).exists())
            self.assertIn('Уборка TEMP: оставлен каталог', log.getvalue())

    def test_persistent_lock_keeps_success(self):
        with tempfile.TemporaryDirectory() as parent:
            log = io.StringIO()
            with contextlib.redirect_stderr(log), patch.object(tempfile.TemporaryDirectory, '_rmtree', side_effect=PermissionError('занят')), patch('temp_cleanup.time.sleep'):
                with TemporaryDirectory(dir=parent) as folder:
                    (Path(folder)/'test.exe').write_bytes(b'passed')
            self.assertTrue(Path(folder).exists())
            self.assertIn(folder, log.getvalue())


class RunnerTests(unittest.TestCase):
    def test_pool_isolation_order_and_failures(self):
        with TemporaryDirectory() as folder:
            root = Path(folder)
            (root/'Tests').mkdir()
            shutil.copy2(Path(__file__).with_name('temp_cleanup.py'), root/'Tests/temp_cleanup.py')
            script = root/'probe.py'
            script.write_text("""import json, os, sys, tempfile, time
from pathlib import Path
time.sleep(float(sys.argv[1]))
scope = Path(os.environ['DOMOVOI_COMPARE_BUILD'])
(scope/'probe.json').write_text(json.dumps({'temp':tempfile.gettempdir(), 'scope':str(scope)}))
print(sys.argv[2])
sys.exit(int(sys.argv[3]))
""", encoding='utf-8')
            jobs = [('slow', [str(script), '0.1', 'PASS', '0'], False),
                    ('exit', [str(script), '0', 'PASS', '3'], False),
                    ('marker', [str(script), '0', 'FAIL sentinel', '0'], False)]
            env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', PYTHONIOENCODING='utf-8')
            log = io.StringIO()
            with contextlib.redirect_stdout(log):
                results = run_groups(jobs, root, root/'build', 'Release', env, 3)
            self.assertEqual(list(results), ['slow', 'exit', 'marker'])
            self.assertEqual([r['status'] for r in results.values()], ['PASS', 'FAIL', 'FAIL'])
            self.assertLess(log.getvalue().index('PASS slow'), log.getvalue().index('FAIL exit'))
            scopes = []
            for name, _, _ in jobs:
                probe = json.loads((root/'build/groups'/name/'probe.json').read_text())
                scopes.append(probe['scope'])
                self.assertEqual(Path(probe['temp']).parent, Path(probe['scope']))
                self.assertFalse(Path(probe['temp']).exists())
            self.assertEqual(len(set(scopes)), 3)


if __name__ == '__main__':
    unittest.main()
