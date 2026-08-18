# Сервер для игры "DogStory"

"DogStory" — многопользовательская игра про собак.<br>
Цель игры собирать предметы по карте и относить их на базу.<br>
На выбор доступно 3 карты.

![Выбор карты](pictures/choose%20map.png)

Предметы нужно относить к зелёному дому с рожком наверху.

![Игра](pictures/game.png)

Игра автоматически завершится при бездействии нескольких секунд и выведется таблица рекордов.

![Таблица рекордов](pictures/records.png)

### Требования
- GCC: 13+
- CMake: 3.15+
- Conan: 2.x
- Python: 3.10+
- PostgreSQL: 15+

### Клонирование репозитория
```bash
git clone
cd DogStory
```

### Docker
```bash
docker build -t dogstory .

docker run --rm -it -p 8080:8080 \
  -e GAME_DB_URL="postgresql://user:pass@host:5432/db" \
  -v $(pwd)/data:/app/data \
  -v $(pwd)/static:/app/static \
  dogstory
```

### Установка Conan
```bash
python3 -m venv venv
source venv/bin/activate
pip install conan
conan profile detect
```

### Сборка проекта:
```bash
# Очистить старую сборку (если есть)
rm -rf build
mkdir -p build && cd build

# Установить зависимости через Conan
conan install .. --build=missing -s build_type=Release --output-folder=.

# Настроить CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# Собрать проект
cmake --build . -j $(nproc)
```

### Аргументы командной строки:
<table>
  <tr>
    <th>Аргумент</th>
    <th>Сокращение</th>
    <th>Описание</th>
  </tr>
  <tr>
    <td><code>--help</code></td>
    <td><code>-h</code></td>
    <td>Показать справку</td>
  </tr>
  <tr>
    <td><code>--tick-period</code></td>
    <td><code>-t</code></td>
    <td>Период обновления мира (мс)</td>
  </tr>
  <tr>
    <td><code>--config-file</code></td>
    <td><code>-c</code></td>
    <td>Путь к конфигу</td>
  </tr>
  <tr>
    <td><code>--www-root</code></td>
    <td><code>-w</code></td>
    <td>Папка со статикой</td>
  </tr>
  <tr>
    <td><code>--randomize-spawn-points</code></td>
    <td>—</td>
    <td>Спавн игроков в случайных местах</td>
  </tr>
  <tr>
    <td><code>--state-file</code></td>
    <td>—</td>
    <td>Файл состояния</td>
  </tr>
  <tr>
    <td><code>--save-state-period</code></td>
    <td>—</td>
    <td>Период автосохранения (мс)</td>
  </tr>
</table>

<h2>Переменные окружения</h2>
<table>
  <tr>
    <th>Переменная</th>
    <th>Описание</th>
    <th>Пример</th>
  </tr>
  <tr>
    <td><code>GAME_DB_URL</code></td>
    <td>Подключение к PostgreSQL</td>
    <td><code>postgresql://user:pass@localhost:5432/db</code></td>
  </tr>
</table>

### Запуск
PostgreSQL:
```bash
# Через Docker
docker run -d \
  --name postgres \
  -e POSTGRES_USER=gameuser \
  -e POSTGRES_PASSWORD=password \
  -e POSTGRES_DB=gamedb \
  -p 5432:5432 \
  postgres:15

# Или через локальный PostgreSQL
sudo systemctl start postgresql
```
Сервер:
```bash
GAME_DB_URL="postgresql://gameuser:password@localhost:5432/gamedb" \
  ./game_server \
    --config-file ../data/config.json \
    --www-root ../static \
    --tick-period 50 \
    --randomize-spawn-points
```
