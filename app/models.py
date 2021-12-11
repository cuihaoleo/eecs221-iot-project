from sqlalchemy import func
from flask_sqlalchemy import SQLAlchemy
db = SQLAlchemy()


class Trashcan(db.Model):
    id = db.Column(db.Integer, primary_key=True, autoincrement=False)
    history = db.relationship("History")


class History(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime(timezone=True), nullable=False, server_default=func.now())
    trashcan = db.Column(db.Integer, db.ForeignKey(Trashcan.id), nullable=False)
    weight = db.Column(db.Integer, nullable=False)
    utilization = db.Column(db.Integer, nullable=False)
    new_item = db.Column(db.String(10), nullable=True)
    image = db.Column(db.String(32), nullable=False)
