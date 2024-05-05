from typing import Any

class SQLiteConnection:
    def connect(self, uri: str) -> None: ...
    def disconnect(self) -> None: ...
    def fetch_all(self, query: str, params: list[Any] = []) -> list[dict]: ...

class PostgresConnection:
    def connect(self, uri: str) -> None: ...
    def disconnect(self) -> None: ...
    def fetch_all(self, query: str, params: list[Any] = []) -> list[dict]: ...
