import os
from http.server import SimpleHTTPRequestHandler
from wsgiref.simple_server import demo_app
import socketserver

WEB_DIR = os.path.dirname(os.path.abspath(__file__))

def application(environ, start_response):
    path = environ.get("PATH_INFO", "/")
    if path == "/" or path == "":
        path = "/index.html"

    file_path = os.path.join(WEB_DIR, path.lstrip("/"))

    if not os.path.isfile(file_path):
        start_response("404 Not Found", [("Content-Type", "text/plain")])
        return [b"404 Not Found"]

    ext = os.path.splitext(file_path)[1]
    content_types = {
        ".html": "text/html; charset=utf-8",
        ".css": "text/css",
        ".js": "application/javascript",
        ".png": "image/png",
        ".jpg": "image/jpeg",
        ".svg": "image/svg+xml",
        ".ico": "image/x-icon",
    }
    content_type = content_types.get(ext, "application/octet-stream")

    with open(file_path, "rb") as f:
        body = f.read()

    start_response("200 OK", [
        ("Content-Type", content_type),
        ("Content-Length", str(len(body))),
    ])
    return [body]
