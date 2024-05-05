# Pythonic Core - Extension in C++
> This is a package for database connections using PostgreSQL.

An extension for connecting to the database written in c/c++ to be used in Python, a performative way of making SQL queries.

## Run local

### Libs postgresql:

```sh
sudo apt-get install postgresql postgresql-client libpq-dev
sudo apt-get install libpq-dev
```

### Build:
```sh
python setup.py bdist_wheel
pip install dist/pythonic_core-0.1-cp312-cp312-linux_x86_64.whl --force-reinstall
```
or
```sh
make local/publish
```

## Examples in Python:

### Example with postgres:

```python
import pythonic_core

db = pythonic_core.PostgresConnection()
db.connect(
    "postgres://default:YqQ40GEUSxBL@ep-raspy-firefly-a4unwdoj-pooler.us-east-1.aws.neon.tech:5432/verceldb?sslmode=require"
)
r = db.fetch_all("select * from interaction", [])
for i in r:
    print(i["created_at"].day)

db.disconnect()
```

### Example with sqlite:
```python
import pythonic_core

db = pythonic_core.SQLiteConnection()
db.connect("example.db")
r = db.fetch_all("select * from users")
for i in r:
    print(i)

db.disconnect()
```
