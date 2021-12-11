import base64
import csv
import datetime
import hashlib
import io
import os
import struct

from flask import (
    Blueprint,
    Response,
    current_app,
    render_template,
    request,
    send_from_directory,
)
from google.cloud import vision
import matplotlib as mpl
import pandas as pd

from app.models import History, Trashcan, db


mpl.use('Agg')
main = Blueprint('main', __name__)


@main.route('/')
def index():
    return render_template('home.html', cans=Trashcan.query.all())


@main.route('/history/<int:device_id>', methods=['GET'])
def history(device_id):
    import matplotlib.pyplot as plt
    device = Trashcan.query.get(device_id)

    fig = plt.figure(figsize=(12, 4))
    ax = fig.add_subplot(111)
    twin_ax = ax.twinx()

    x = [h.timestamp for h in device.history]
    x.append(datetime.datetime.utcnow())
    y = [h.weight for h in device.history]
    y.append(y[-1])
    ax.plot(x, y, 'r', label="weight")
    ax.legend(loc="upper left")
    ax.set_ylabel("weight/g")
    y = [h.utilization for h in device.history]
    y.append(y[-1])
    twin_ax.plot(x, y, "b", label="utilization")
    twin_ax.legend(loc="lower right")
    twin_ax.set_ylabel("utilization%")

    with io.BytesIO() as f:
        fig.savefig(f, format='png')
        png_data_encoded = base64.b64encode(f.getbuffer())
        img_embed = 'data:image/png;base64,{}'.format(png_data_encoded.decode())

    return render_template('history.html', device=device, img_embed=img_embed)


@main.route('/images/<path:filename>', methods=['GET'])
def images(filename):
    return send_from_directory(current_app.config['UPLOAD_FOLDER'], filename)


@main.route('/update', methods=['POST'])
def update():
    def map_waste_type(google_label):
        if google_label in ["Fruit", "Squash", "Food", "Produce", "Ball"]:
            return "Food"
        else:
            return "Non-Food"

    def check_duplicated(new_item, old_items):
        def overlap_area(box1, box2):
            dx = min(box1.x1, box2.x1) - max(box1.x0, box2.x0)
            dy = min(box1.y1, box2.y1) - max(box1.y0, box2.y0)
            return dx * dy if dx >= 0 and dy >= 0 else 0

        for _, old_item in old_items.iterrows():
            if old_item["name"] == new_item["name"]:
                o_area = overlap_area(old_item, new_item)
                new_item_area = (new_item.y1 - new_item.y0) * (new_item.x1 - new_item.x0)

                if o_area / new_item_area > 0.5:
                    return True

        return False

    upload_folder = current_app.config['UPLOAD_FOLDER']
    vision_client = current_app.config['VISION_CLIENT']

    raw_data = request.get_data()
    device_id, weight, utilization = struct.unpack("<LHH", raw_data[0:8])
    img_data = raw_data[8:]

    filename = hashlib.sha224(img_data).hexdigest()
    response = vision_client.object_localization(image=vision.Image(content=img_data))

    if (device := Trashcan.query.get(device_id)) is None:
        device = Trashcan(id=device_id)
        db.session.add(device)
        db.session.commit()

    if len(device.history) > 0:
        last_history = device.history[-1]
        last_objects = pd.read_csv(os.path.join(upload_folder, last_history.image + ".csv"), na_filter=False)
    else:
        last_objects = pd.DataFrame()

    new_object = None

    with open(os.path.join(upload_folder, filename + ".csv"), "w", newline="") as fout:
        writer = csv.DictWriter(fout, fieldnames=["name", "score", "x0", "y0", "x1", "y1"])
        writer.writeheader()

        for item in response.localized_object_annotations:
            row = {
                "name": item.name,
                "score": item.score,
                "x0": item.bounding_poly.normalized_vertices[0].x,
                "y0": item.bounding_poly.normalized_vertices[0].y,
                "x1": item.bounding_poly.normalized_vertices[2].x,
                "y1": item.bounding_poly.normalized_vertices[2].y,
            }
            writer.writerow(row)

            if new_object is None:
                this_object = pd.Series(row)
                if not check_duplicated(this_object, last_objects):
                    new_object = map_waste_type(item.name)

    with open(os.path.join(upload_folder, filename + ".jpg"), "wb") as fout:
        fout.write(raw_data[8:])

    history = History(trashcan=device.id, weight=weight, utilization=utilization, image=filename)
    history.new_item = new_object
    db.session.add(history)
    db.session.commit()

    if new_object == 'Food':
        return Response(b'\xff', mimetype='application/octet-stream')
    else:
        return Response(b'\x00', mimetype='application/octet-stream')
