from http.server import HTTPServer
from server import CaptionServerHandler

if __name__ == "__main__":
    server = HTTPServer(("localhost", 8080), CaptionServerHandler)
    server.serve_forever()
