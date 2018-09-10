#!/bin/env python
from distutils.core import setup, Extension
import os

src=["c/Chessmodule.c",
     "c/mgenerator.c",
     "c/eval.c",
     "c/search.c"]

scripts=["ClientServerChess.py", 
         "SimpleHTTPChessServer.py",
         "cgiskak.py",
         "cgireview.py"]

inc=[]

defines=[]
if os.name=="nt":
    defines+=[("VISUALCPP",1)]

setup(name="PChess",
      version="1.0.4",
      maintainer="Jesper Olsen",
      maintainer_email="Chess@JesperOlsen.Net",
      license="MIT",
      download_url="http://JesperOlsen.Net/Balls/PChess.zip",
      description="Chess engine",
      py_modules=["PChess"],
      scripts=scripts,
      ext_modules=[Extension("Chess", sources=src, include_dirs=inc,
                                      define_macros=defines)
                  ]
     )
