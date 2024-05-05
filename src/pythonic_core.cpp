#include <pybind11/pybind11.h>
#include <pybind11/chrono.h>
#include <datetime.h>
#include <stdexcept>
#include <string>
#include <regex>

#include <libpq-fe.h>
#include <sqlite3.h>

namespace py = pybind11;

void init_datetime_module()
{
    PyDateTime_IMPORT;
}

PyObject *create_datetime_from_string(const char *s)
{
    int year, month, day, hour, minute, second;
    if (sscanf(s, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6)
    {
        return PyDateTime_FromDateAndTime(year, month, day, hour, minute, second, 0);
    }
    return NULL;
}

PyObject *create_date_from_string(const char *s)
{
    int year, month, day;
    if (sscanf(s, "%d-%d-%d", &year, &month, &day) == 3)
    {
        return PyDate_FromDate(year, month, day);
    }
    return NULL;
}

PyObject *create_delta_from_string(const char *s)
{
    int days, seconds, microseconds;
    if (sscanf(s, "%d %d %d", &days, &seconds, &microseconds) == 3)
    {
        return PyDelta_FromDSU(days, seconds, microseconds);
    }
    return NULL;
}

class DatabaseConnection
{
public:
    virtual void connect(const std::string &uri) = 0;
    virtual void disconnect() = 0;
    virtual py::list fetch_all(const std::string &query, const py::list &py_params = py::list()) = 0;
    virtual ~DatabaseConnection() {}
};

class PostgresConnection : public DatabaseConnection
{
private:
    PGconn *conn_;

public:
    void connect(const std::string &uri) override
    {
        conn_ = PQconnectdb(uri.c_str());
        if (PQstatus(conn_) != CONNECTION_OK)
        {
            throw std::runtime_error("Connection to database failed: " + std::string(PQerrorMessage(conn_)));
        }
    }

    void disconnect() override
    {
        if (conn_ != nullptr)
        {
            PQfinish(conn_);
            conn_ = nullptr;
        }
    }

    py::list fetch_all(const std::string &query, const py::list &py_params = py::list()) override
    {
        const char **params = nullptr;
        int n_params = py_params.size();
        std::vector<std::string> param_values(n_params);
        if (!py_params.empty())
        {
            params = new const char *[n_params];
            for (size_t i = 0; i < n_params; ++i)
            {
                param_values[i] = py_params[i].cast<std::string>();
                params[i] = param_values[i].c_str();
            }
        }

        PGresult *res = PQexecParams(conn_, query.c_str(), n_params, nullptr, params, nullptr, nullptr, 0);
        delete[] params;

        if (PQresultStatus(res) != PGRES_TUPLES_OK)
        {
            PQclear(res);
            throw std::runtime_error("Query failed: " + std::string(PQerrorMessage(conn_)));
        }

        py::list result_list;
        int nrows = PQntuples(res);
        for (int i = 0; i < nrows; ++i)
        {
            py::dict row_dict;
            for (int j = 0; j < PQnfields(res); ++j)
            {
                const char *col_name = PQfname(res, j);
                Oid type = PQftype(res, j);
                const char *val = PQgetvalue(res, i, j);
                py::object py_val;

                switch (type)
                {
                case 23:
                    py_val = py::int_(std::stoi(val));
                    break;
                case 700:
                case 701:
                    py_val = py::float_(std::stod(val));
                    break;
                case 16:
                    py_val = py::bool_(std::string(val) == "t");
                    break;
                case 1082:
                    py_val = py::module::import("datetime").attr("date").attr("fromisoformat")(std::string(val));
                    break;
                case 1114:
                case 1184:
                    py_val = py::module::import("datetime").attr("datetime").attr("fromisoformat")(std::string(val));
                    break;
                case 1186:
                    py_val = py::module::import("datetime").attr("timedelta").attr("fromisoformat")(std::string(val));
                    break;
                default:
                    py_val = py::str(val);
                }

                row_dict[col_name] = py_val;
            }
            result_list.append(row_dict);
        }

        PQclear(res);
        return result_list;
    }
};

class SQLiteConnection : public DatabaseConnection
{
private:
    sqlite3 *conn_;

public:
    void connect(const std::string &uri) override
    {
        if (sqlite3_open(uri.c_str(), &conn_) != SQLITE_OK)
        {
            throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(conn_)));
        }
    }

    void disconnect() override
    {
        if (conn_ != nullptr)
        {
            sqlite3_close(conn_);
            conn_ = nullptr;
        }
    }

    py::list fetch_all(const std::string &query, const py::list &py_params = py::list()) override
    {
        py::list result_list;
        sqlite3_stmt *stmt;
        const char *tail;

        if (sqlite3_prepare_v2(conn_, query.c_str(), -1, &stmt, &tail) != SQLITE_OK)
        {
            throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(conn_)));
        }

        if (!py_params.empty())
        {
            for (int i = 0; i < py_params.size(); ++i)
            {
                const auto &item = py_params[i];
                if (item.is_none())
                {
                    sqlite3_bind_null(stmt, i + 1);
                }
                else if (py::isinstance<py::int_>(item))
                {
                    sqlite3_bind_int(stmt, i + 1, item.cast<int>());
                }
                else if (py::isinstance<py::str>(item))
                {
                    sqlite3_bind_text(stmt, i + 1, item.cast<std::string>().c_str(), -1, SQLITE_TRANSIENT);
                }
                else if (py::isinstance<py::float_>(item))
                {
                    sqlite3_bind_double(stmt, i + 1, item.cast<double>());
                }
                else
                {
                    throw std::runtime_error("Unsupported parameter type");
                }
            }
        }

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            py::dict row_dict;
            int ncols = sqlite3_column_count(stmt);
            for (int i = 0; i < ncols; i++)
            {
                const char *col_name = sqlite3_column_name(stmt, i);
                switch (sqlite3_column_type(stmt, i))
                {
                case SQLITE_INTEGER:
                    row_dict[col_name] = sqlite3_column_int(stmt, i);
                    break;
                case SQLITE_FLOAT:
                    row_dict[col_name] = sqlite3_column_double(stmt, i);
                    break;
                case SQLITE_TEXT:
                {
                    std::string value = reinterpret_cast<const char *>(sqlite3_column_text(stmt, i));
                    std::regex date_regex("^(\\d{4}-\\d{2}-\\d{2})$");                          // Regex para datas YYYY-MM-DD
                    std::regex datetime_regex("^(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2})$"); // Regex para timestamps YYYY-MM-DD HH:MM:SS
                    if (std::regex_match(value, date_regex))
                    {
                        py::object py_date = py::module::import("datetime").attr("date").attr("fromisoformat")(value);
                        row_dict[col_name] = py_date;
                    }
                    else if (std::regex_match(value, datetime_regex))
                    {
                        py::object py_datetime = py::module::import("datetime").attr("datetime").attr("fromisoformat")(value);
                        row_dict[col_name] = py_datetime;
                    }
                    else
                    {
                        row_dict[col_name] = value;
                    }
                    break;
                }
                case SQLITE_NULL:
                    row_dict[col_name] = py::none();
                    break;
                default:
                    row_dict[col_name] = py::str(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));
                }
            }
            result_list.append(row_dict);
        }

        sqlite3_finalize(stmt);
        return result_list;
    }
};

PYBIND11_MODULE(pythonic_core, m)
{
    py::class_<DatabaseConnection, std::shared_ptr<DatabaseConnection>>(m, "DatabaseConnection")
        .def("connect", &DatabaseConnection::connect)
        .def("disconnect", &DatabaseConnection::disconnect)
        .def("fetch_all", &DatabaseConnection::fetch_all);

    py::class_<PostgresConnection, DatabaseConnection, std::shared_ptr<PostgresConnection>>(m, "PostgresConnection")
        .def(py::init<>())
        .def("connect", &PostgresConnection::connect)
        .def("disconnect", &PostgresConnection::disconnect)
        .def("fetch_all", &PostgresConnection::fetch_all, py::arg("query"), py::arg("py_params") = py::list());

    py::class_<SQLiteConnection, DatabaseConnection, std::shared_ptr<SQLiteConnection>>(m, "SQLiteConnection")
        .def(py::init<>())
        .def("connect", &SQLiteConnection::connect)
        .def("disconnect", &SQLiteConnection::disconnect)
        .def("fetch_all", &SQLiteConnection::fetch_all, py::arg("query"), py::arg("py_params") = py::list());
}
