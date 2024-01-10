from http.server import BaseHTTPRequestHandler
import logging
import cgi
from io import BytesIO
from caption import Caption, show_caption, clear_caption

logging.getLogger().setLevel(logging.INFO)


def _parse_caption_form_request(data: dict) -> Caption:
    required_keys = ["image", "x", "y"]
    for key in required_keys:
        if key not in data or len(data[key]) == 0:
            raise Exception(f"Missing key: {key}")

    image = BytesIO(data["image"][0])
    x = int(data["x"][0])
    y = int(data["y"][0])
    return Caption(image, x, y)


class CaptionServerHandler(BaseHTTPRequestHandler):
    def _set_status_and_headers(self, status: int):
        self.send_response(status)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, DELETE, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type")
        self.end_headers()

    def do_OPTIONS(self):
        self._set_status_and_headers(204)

    def do_DELETE(self):
        try:
            clear_caption()
        except Exception as e:
            logging.info(f"Error clearing caption: {e}")
            self._set_status_and_headers(500)
            return
        self._set_status_and_headers(204)

    def do_POST(self):
        pdict: dict
        content_type, pdict = cgi.parse_header(self.headers["content-type"])

        if content_type != "multipart/form-data":
            logging.info(f"Unexpected content type: {content_type}")
            self._set_status_and_headers(415)
            return

        pdict["boundary"] = bytes(pdict["boundary"], "utf-8")
        form_data = cgi.parse_multipart(self.rfile, pdict)
        try:
            caption = _parse_caption_form_request(form_data)
        except Exception as e:
            logging.info(f"Error parsing form data: {e}")
            self._set_status_and_headers(400)
            return

        logging.info(f"Received caption: {caption}")

        try:
            show_caption(caption)
        except Exception as e:
            logging.info(f"Error showing caption: {e}")
            self._set_status_and_headers(500)
            return

        self._set_status_and_headers(204)
