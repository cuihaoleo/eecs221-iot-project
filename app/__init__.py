import os

from flask import Flask
from flask_migrate import Migrate
from google.cloud import vision

from app import views
from app.models import db


BASEDIR = os.path.dirname(os.path.realpath(__file__))


def create_app():
    app = Flask(__name__)
    app.config['SQLALCHEMY_DATABASE_URI'] = f"sqlite:///{app.root_path}/database.db"
    app.config['UPLOAD_FOLDER'] = upload_folder = os.path.join(app.root_path, "images")
    os.makedirs(upload_folder, exist_ok=True)

    os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = "key.json"
    app.config['VISION_CLIENT'] = vision.ImageAnnotatorClient()

    db.init_app(app)
    Migrate(app, db)

    app.register_blueprint(views.main, url_prefix='')

    return app
