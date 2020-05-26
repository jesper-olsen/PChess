# PChess

![alt text](https://github.com/jesper-olsen/PChess/images/PChess.png "Game UI")

PChess is a chess engine, which I originally wrote as part of my MSc thesis.                      The engine performs "brute force" search, with a minimum of chess specific knowledge to speed up the search. The search is extended iteratively until a certain depth is reached and/or a certain number of positions have been investigated.                          

The engine respects the usual termination rules:                          
* Check mate                                                  
* Stale mate (draw)                                         
* Draw by three times repetition                            
* Draw by the 50 move rule

This updated version of the project runs as a python flask app. 
One way to deploy the app is through Docker:

Dockerfile
```
FROM python:3.8-alpine
RUN apk add build-base
RUN adduser -D pchess
WORKDIR /home/pchess
COPY PChess PChess
RUN python -m venv venv
RUN venv/bin/pip install -r PChess/requirements.txt
RUN venv/bin/pip install gunicorn
COPY boot.sh ./
RUN chmod +x boot.sh
ENV FLASK_APP PChess.py
RUN chown -R pchess:pchess ./
USER pchess
EXPOSE 5000
ENTRYPOINT ["./boot.sh"]
```

boot.sh
```
#!/bin/sh
source venv/bin/activate
cd PChess; make chess.so
exec gunicorn -b :5000 --access-logfile - --error-logfile - PChess:app
```

Build and run with docker:
```
docker build -t pchess:latest .
```
```
docker run --name pchess -p 8000:5000 --rm pchess:latest
```
