from flask import Flask
# from flask_cors import CORS
from flask_sqlalchemy import SQLAlchemy

app=Flask('accesscont')

app.config.from_pyfile('settings.py')

# cors = CORS(app)
db=SQLAlchemy(app)

from accesscont import views,commands
