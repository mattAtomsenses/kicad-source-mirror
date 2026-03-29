from http.server import HTTPServer, SimpleHTTPRequestHandler
import os

PORT = 5000
WEB_DIR = os.path.dirname(os.path.abspath(__file__))

class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def log_message(self, format, *args):
        print(f"[{self.address_string()}] {format % args}")

if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", PORT), Handler)
    print(f"Serving KiCad project info at http://0.0.0.0:{PORT}")
    server.serve_forever()
