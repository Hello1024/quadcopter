#!/usr/bin/env qlua

-- This program is a basic GUI to annotate training data, saving the resulting network in "trainingdata".

-- click on the point to add a training point.  Click in the top left to say there is no point in this frame.

-- It uses a modified version of 'ffmpeg' to fix errors on ubuntu.

require 'camera'
require 'image'
require 'optim'
require 'ffmpeg'
require 'qt'

local file_name = "testdata1"   -- name of input and output files

-- update values here too - it isn't smart enough to calculate them :-(
cam = ffmpeg.Video{path=file_name..'.webm', width=640, height=360, fps=30, length=115, load=false}
cam.n_channels = 1

local window, painter
window, painter = image.window()

local results = torch.load(file_name..'.annotations', 'ascii')
local frame = 0

qt.connect(window.listener,
                 'sigMousePress(int,int,QByteArray,QByteArray,QByteArray)',
                 function (x,y,...)
                   
                   results[frame] = {x=x, y=y}
                   while results[frame] do
                     if results[frame].x < 10 and results[frame].y < 10 then
                       print("moo")
                       -- No quadcopter in this frame
                       results[frame].x = -1
                       results[frame].y = -1
                     end
                     frame = frame+1
                     print(frame)
                     torch.save(file_name..'.annotations', results , 'ascii')
                     dd = image.toDisplayTensor{input={cam:forward()},padding=1, scaleeach=true}
                     image.display{image=dd, win=painter}
                   end
                 end)
window:show()



