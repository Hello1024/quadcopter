#!/usr/bin/env qlua

-- This program is a basic GUI to annotate training data, saving the resulting network in "trainingdata".

-- It uses a modified version of 'ffmpeg' to fix errors on ubuntu.

require 'camera'
require 'image'
require 'optim'
require 'ffmpeg'
require 'qt'


cam = ffmpeg.Video{path='testdata1.webm', width=640, height=360, fps=30, length=115, load=false}
cam.n_channels = 1

local window, painter
window, painter = image.window()

local results = torch.load("trainingdata", 'ascii')
local frame = 0

qt.connect(window.listener,
                 'sigMousePress(int,int,QByteArray,QByteArray,QByteArray)',
                 function (x,y,...)
                   
                   results[frame] = {x=x, y=y}
                   while results[frame] do
                     frame = frame+1
                     print(frame)
                     torch.save("trainingdata", results , 'ascii')
                     dd = image.toDisplayTensor{input={cam:forward()},padding=1, scaleeach=true}
                     image.display{image=dd, win=painter}
                   end
                 end)
window:show()



