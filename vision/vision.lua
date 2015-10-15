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

local criterion = nn.MSECriterion() 
local cam
cam = image.Camera(0,640,480,2,30)

local input = cam:forward()
local lastframe
local framecount = 0

local window, painter
window, painter = image.window()

local net = torch.load("net")
local frame = 0

window:show()

quad_state = torch.rand(torch.LongStorage{1000, 9, 2})

-- quad_state occupancy table:
-- ---------------------------------------------------------------
-- | probability | x | y | z | heading | dx | dy | dz | dheading |
-- |                                                             |
-- 2nd pane is all the same values, but stddev probability distributions.
-- all values are 0 <= x < 1.0


while true do
  lastframe = input:clone()

  input = cam:forward()

  
  local inp = {input, lastframe}
  local netout = net:forward(inp)
  
  for i=1,quad_state:size(1) do
    quad_state[i][1][1] = quad_state[i][1][1] + netout[torch.floor(quad_state[i][2][1]*netout:size(1))+1][torch.floor(quad_state[i][3][1]*netout:size(2))+1]
  end
  
  
  
  
  print("Out Max: " .. netout:max())
  print("Out Min: " .. netout:min())
  
  -- netout:add(validoutput)
  dd = image.toDisplayTensor{input={netout, validoutput},padding=1, scaleeach=true}
  image.display{image=dd, win=painter}
  
end


