# Окружение локальных тестов

Проверено на Windows x64: CPython **3.11.9**, `unicorn==2.1.4`,
`pyelftools==0.32`. Установка зависимостей привязана к выбранному Python:

```powershell
py -3.11 -m venv Release/test-venv
& Release/test-venv/Scripts/python.exe -m pip install -r Tests/requirements.txt
& Release/test-venv/Scripts/python.exe Tests/test_fault_capture.py
& Release/test-venv/Scripts/python.exe Tests/test_iar_photo_heap.py
```

Тесты IAR требуют предварительной сборки Release и используют её свежие
`Release/Exe/DomovoiNext.{out,bin}` и `Release/List/DomovoiNext.map`.
Unicorn исполняет инструкции прошивки в модели; плата, COM-порты и сервер
не используются. Для Python 3.12 создайте отдельное окружение тем же способом;
его запуск в этой проверке не подтверждён.

Host-тесты `Tests/run_host.py` и `Bootloader/tests/run_tests.py` используют Zig
0.13.0 из настроенного в проекте пути. Для загрузчика передайте путь через
`--cc`. Глобальный Python и каталоги зависимостей основной копии не меняются.
