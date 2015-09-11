#!/usr/bin/env qlua

-- This module trains a vision convnet, saving the resulting network in a file.

-- you probably want to run with LD_LIBRARY_PATH=/usr/local/cuda-6.5/lib64 ./vision-train.lua

require 'nngraph'
require 'camera'
require 'image'
require 'optim'
require 'ffmpeg'
require 'qt'
require 'cunn'

-- generate SVG of the graph with the problem node highlighted
-- and hover over the nodes in svg to see the filename:line_number info
-- nodes will be annotated with local variable names even if debug mode is not enabled.
nngraph.setDebug(true)

local function get_net(from, to)

    local input_x = nn.Identity()()
    local input_y = nn.Identity()()
    local joined = nn.JoinTable(1)({input_x, input_y})
    
    local L1 = nn.SpatialMaxPooling(7,7,1,1,3,3)(nn.ReLU()(nn.Dropout()(nn.SpatialConvolution(6, 4, 3, 3,1,1)(joined))))
    local L2 = nn.SpatialMaxPooling(7,7,1,1,3,3)(nn.ReLU()(nn.Dropout()(nn.SpatialConvolution(4, 3, 3, 3,1,1)(L1))))
    local L3 = nn.SpatialConvolution(3, 1, 3, 3,1,1)(L2)

    return nn.gModule({input_x, input_y},{L3})
end

local net = get_net():cuda()
net:zeroGradParameters()
local criterion = nn.MSECriterion():cuda()
local cam = ffmpeg.Video{path='testdata1.webm', width=640, height=360, fps=30, length=30, load=false}
cam.n_channels = 1

local trainer = nn.StochasticGradient(net, criterion)
trainer.learningRate = 0.01

local input = cam:forward():cuda()
local lastframe = input:clone()
local framecount = 0

local window, painter
window, painter = image.window()

local results = torch.load("testdata1.annotations", 'ascii')
local frame = 1

window:show()

while true do
  repeat 
    lastframe:copy(input)
    frame = cam.current
    input:copy(cam:forward())
  until (results[frame] and results[frame].x >= 0)
  
  local inp = {input, lastframe}
  local netout = net:forward(inp)
  

  validoutput = torch.Tensor(netout:size()):squeeze():zero()

  image.gaussian(0, 0, 1, false, 0, 0, 0.02, 0.02, results[frame].x/640, results[frame].y/360, validoutput)
  
  validoutput:add(-0.5)
  validoutput = validoutput:cuda()
  
  --trainer:train(dataset)

  out = criterion:forward(netout, validoutput)
  
  local critback = criterion:backward(netout, validoutput)
  
  net:backward(inp, critback)
  
  net:updateParameters(0.1)
  net:zeroGradParameters()
  
  print("loss: " .. out)
  print("Target Max: " .. validoutput:max())
  print("Target Min: " .. validoutput:min())
  print("Out Max: " .. netout:max())
  print("Out Min: " .. netout:min())
  
  -- netout:add(validoutput)
  if frame > 100 then
    dd = image.toDisplayTensor{input={netout:float(), validoutput:float()},padding=1, scaleeach=true}
    image.display{image=dd, win=painter}
  end
  
  print(frame)
end


