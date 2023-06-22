from flask import Flask

from flask_sqlalchemy import SQLAlchemy

app=Flask('accesscont')

app.config.from_pyfile('settings.py')

db=SQLAlchemy(app)

from accesscont import views,commands
