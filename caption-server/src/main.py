from http.server import HTTPServer
from server import CaptionServerHandler
import logging

if __name__ == "__main__":
    host, port = "", 8080
    server = HTTPServer((host, port), CaptionServerHandler)

    logging.info(f"Server started on port {port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    server.server_close()
    logging.info(f"Server stopped")
