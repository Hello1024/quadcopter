import numpy as np
import tensorflow as tf
import cv2
import pickle
import time
from scipy.ndimage.filters import gaussian_filter
#import matplotlib
#matplotlib.use('GTK')
import matplotlib.pyplot as plt
import particle
import control
import thread
import os
from tensorflow.python.client import timeline

def weight_variable(shape):
  initial = tf.truncated_normal(shape, stddev=0.1)
  return tf.Variable(initial)

def bias_variable(shape):
  initial = tf.constant(0.1, shape=shape)
  return tf.Variable(initial)

def conv2d(x, W):
  return tf.nn.conv2d(x, W, strides=[1, 1, 1, 1], padding='SAME')

def max_pool_2x2(x):
  return tf.nn.max_pool(x, ksize=[1, 2, 2, 1],
                        strides=[1, 2, 2, 1], padding='SAME')

def layer(inp, noutlayers=5):
  W = weight_variable([5, 5, inp.get_shape().dims[3].value, noutlayers])
  b = bias_variable([noutlayers])
  
  mean, var = tf.nn.moments(inp, [0,1,2,3])

  out = tf.nn.relu(conv2d((inp-mean)/tf.sqrt(var), W) + b)
  #out = tf.nn.dropout(out, 0.5)
  return max_pool_2x2(out)

def getModel(inp):
  with tf.name_scope("gen") as scope:
    layers = []
    mean, var = tf.nn.moments(inp, [0,1,2,3])
    inp = (inp-mean)/tf.sqrt(var)
    layers.append(inp)
    while (layers[-1].get_shape().dims[1]>10):
      layers.append(layer(layers[-1], noutlayers=3))
    
    # Combine layers
    shape = [x.value for x in layers[0].get_shape().dims]
    h_w = [shape[1], shape[2]]
    resized = [tf.image.resize_nearest_neighbor(x, h_w) for x in layers]
    combined = tf.concat(3, resized)

    W = weight_variable([1, 1, combined.get_shape().dims[3].value, 2])
    b = bias_variable([2])
    final = conv2d(combined, W) + b

    #return final[:,:,:,0]
    return tf.reshape(tf.nn.softmax(tf.reshape(final, [-1, 2]))[:,0], [shape[0], shape[1], shape[2]])


def click(event, x, y, flags, param):
  global nextframe
  global annotations
  global frame_number
  if event == cv2.EVENT_LBUTTONDOWN:
    annotations[frame_number] = (x,y)
  elif event == cv2.EVENT_RBUTTONDOWN:
    annotations[frame_number] = 0
  else:
    return
  nextframe=1
  pickle.dump( annotations, open( "testdata1.pannotations", "wb" ) )

def remote_control(event, x, y, flags, param):
  global remote_x
  remote_x = (x/320.0-1)
  global remote_y
  remote_y = (y/240.0-1)
  global controls_val
  global serial_connection	

  controls_val[0] = -remote_x
  controls_val[1] = remote_y
  
  serial_connection.update_quad(0, controls_val)

  


def vis(inp):
  inp = np.squeeze(inp).astype(float)
  if inp.ndim == 2:
    inp = np.expand_dims(inp, axis=2)

  inp = (inp - inp.min()) / (inp.max()-inp.min())
  return np.broadcast_to(inp, (inp.shape[0], inp.shape[1], 3))

cap = cv2.VideoCapture(0); #'testdata1.webm')

def keep_capturing():
  global cap
  global cv2
  while True:
    cap.grab()
    cv2.waitKey(1)

thread.start_new_thread( keep_capturing, () )

try:
  annotations = pickle.load( open( "testdata1.pannotations", "rb" ) )
except (OSError, IOError) as e:
  annotations = {}
frame_number = -1
loss_avg = 0

_, frame = cap.read()
inp = tf.placeholder("float", [1, frame.shape[0],frame.shape[1],frame.shape[2]*2])
correct = tf.placeholder("float", [1, frame.shape[0],frame.shape[1]])
controls = tf.placeholder("float", [1, 4])

model = getModel(inp)

saver = tf.train.Saver()

loss = tf.reduce_sum(tf.square(tf.square(model - correct)))   # Not mathematically correct for probabilities...  Oh well!
train_step = tf.train.AdamOptimizer(5e-4).minimize(loss)


part = particle.ParticleFilter();
particlepreview = part.preview_like(tf.squeeze(model))
time_delta = tf.placeholder("float", [])
model_update_op = part.UpdateProbs(tf.squeeze(model), controls)
position_op = part.best_guess()

serial_connection = control.SerialConnection();

sess = tf.Session();
sess.run(tf.initialize_all_variables())
saver.restore(sess, "my-model55044166.9602")

cv2.namedWindow("frame")
cv2.setMouseCallback("frame", remote_control)

eventlog = {}
eventlog['path'] = "logs/"+str(time.time())+"/"
os.makedirs(eventlog['path'])
event_vid = cv2.VideoWriter(eventlog['path'] + 'vid.avi', cv2.cv.CV_FOURCC('m','p','4','v'), 25, (640,480))

print event_vid.isOpened()

options = tf.RunOptions(trace_level=tf.RunOptions.FULL_TRACE)
run_metadata = tf.RunMetadata()

    

t_loop = 0
remote_x = 0
remote_y = 0
controls_val = [0,0,0,0]
    
while(cap.isOpened()):
    frame_number = frame_number + 1
    last = frame
    
    print "Loop time", time.time() - t_loop
    t_delta = time.time() - t_loop
    t_loop = time.time()
    
    ret, frame = cap.retrieve()
    event_vid.write(frame)
    eventlog[frame_number] = {}
    eventlog[frame_number]['time'] = t_loop
    
    #if frame_number not in annotations:
    #  #cap = cv2.VideoCapture('testdata1.webm')
    #  _, frame = cap.read()
    #  frame_number=-1
    #  print "reset"
    #  continue
    
    #if frame_number < 95:
    #  continue
    
    correct_val = np.zeros([1, last.shape[0],last.shape[1]])
    try:
      correct_val[0,annotations[frame_number][1], annotations[frame_number][0]] = 100
      #correct_val = gaussian_filter(correct_val, sigma=25)
    except:
      pass
    
    comp_frame = np.expand_dims(np.concatenate([frame, last], 2), axis=0)
    t0 = time.time()
    result, loss_num, particlepreview_val, _, position_val = sess.run([model, loss, particlepreview, model_update_op, position_op], feed_dict={inp: comp_frame, correct: correct_val, time_delta: t_delta, controls: [controls_val]}, options=options, run_metadata=run_metadata)
    t1 = time.time()
    loss_avg = loss_avg*0.99 + loss_num*0.01
    
    print t1-t0, loss_avg
    
    # Estimate of position in 3 frames.
    pos_fut_est = position_val[0] + position_val[3]*4
    
    cv2.waitKey(1)

    controls_val = [-remote_x,remote_y, pos_fut_est*0.5+0.5,0]
    print  controls_val
    eventlog[frame_number]['controls'] = controls_val
    serial_connection.update_quad(0, controls_val)

    #cv2.imshow('frame',np.concatenate([vis(x) for x in [frame, result, correct_val]], 0))
    cv2.imshow('frame', np.concatenate([vis(x) for x in [result, particlepreview_val]], 0))
    
    # Create the Timeline object, and write it to a json file
    fetched_timeline = timeline.Timeline(run_metadata.step_stats)
    chrome_trace = fetched_timeline.generate_chrome_trace_format()
    with open('timeline_01.json', 'w') as f:
        f.write(chrome_trace)
    
    #plt.show(block=False)
    #plt.pause(0.001)

    #if frame_number == 7:
        # Append the step number to the checkpoint name:
    #    saver.save(sess, 'my-model'+str(loss_avg))
        
    #while nextframe==0 and frame_number not in annotations:
    pickle.dump( eventlog, open( eventlog['path'] + "log", "wb" ) )
    
    cv2.waitKey(1)

    


cap.release()
cv2.destroyAllWindows()
