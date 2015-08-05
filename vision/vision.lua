#!/usr/bin/env qlua
-- th

require 'nngraph'
require 'camera'
require 'image'
require 'optim'
require 'ffmpeg'
require 'qt'


-- generate SVG of the graph with the problem node highlighted
-- and hover over the nodes in svg to see the filename:line_number info
-- nodes will be annotated with local variable names even if debug mode is not enabled.
nngraph.setDebug(true)

local function get_net(from, to)

    local input_x = nn.Identity()()
    local input_y = nn.Identity()()
    local joined = nn.JoinTable(1)({input_x, input_y})
    
    local smallj = nn.SpatialAveragePooling(6,6,6,6)(joined)

    local L1 = nn.SpatialMaxPooling(3,3,1,1,1,1)(nn.ReLU()(nn.Dropout()(nn.SpatialConvolution(6, 4, 3, 3,1,1)(joined))))
    
    local smallL1 = nn.SpatialAveragePooling(3,3,3,3)(L1)
    
    local L2 = nn.SpatialMaxPooling(3,3,1,1,1,1)(nn.ReLU()(nn.Dropout()(nn.SpatialConvolution(4, 3, 3, 3,1,1)(L1))))
    
    local smallL2 = nn.SpatialAveragePooling(3,3,3,3)(L2)
    
    local L3 = nn.SpatialConvolution(3, 1, 3, 3,1,1)(L2)

    return nn.gModule({input_x, input_y},{L3})
end

local net = get_net()
local criterion = nn.MSECriterion() 
local cam
if false then
  cam = image.Camera(0,640,480,2,30)
else
  cam = ffmpeg.Video{path='testdata1.webm', width=640, height=360, fps=30}
  cam.n_channels = 1
end
local trainer = nn.StochasticGradient(net, criterion)
trainer.learningRate = 0.01

local input = cam:forward()
local lastframe
local framecount = 0

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

while true do
  lastframe = input:clone()

  frame = cam.current
  input = cam:forward()

  
  
  local inp = {input, lastframe}
  local netout = net:forward(inp)
  
  validoutput = netout:clone()
  
  validoutout = validoutput:squeeze()
  
  image.gaussian(0, 0, 1, false, 0, 0, 0.02, 0.02, results[frame].x/640, results[frame].y/360, validoutout)
  
  validoutput:add(-0.5)
   validoutput:mul(-1)
  
  --trainer:train(dataset)

  out = criterion:forward(netout, validoutput)
  
  
  local critback = criterion:backward(netout, validoutput)
  
  net:backward(inp, critback )
  
  net:updateParameters(0.01)
  net:zeroGradParameters()
  
  print("loss: " .. out)
  print("Target Max: " .. validoutput:max())
  print("Target Min: " .. validoutput:min())
  print("Out Max: " .. netout:max())
  print("Out Min: " .. netout:min())
  
  -- netout:add(validoutput)
  if frame > 100 then
    dd = image.toDisplayTensor{input={netout, validoutput},padding=1, scaleeach=true}
    image.display{image=dd, win=painter}
  end
  
  print(frame)
end


