import numpy as np
import tensorflow as tf
import cv2
import pickle
import time
import matplotlib.pyplot as plt



class ParticleFilter:

  def __init__(self):
    self.particles = 1000
    #                                 x   y   z   dx  dy  dz| unused 
    self.initial_std =  tf.constant([0.3,0.3,0.3,0.0,0.0,0.0,0,0,0,0])
    self.initial_bias = tf.constant([0.0,0,1,0,0,0,0,0,0,0])
    self.update_std =  tf.constant([0.0,0.0,0.0,0.03,0.03,0.03,0,0,0,0])
    
    initial = tf.random_normal([self.particles, 10]) * self.initial_std + self.initial_bias
    self.state = tf.Variable(initial, trainable=False)
    pass

  def project(self):
    """Return all particles perspective projected onto a 2D plane"""
    inp = tf.concat(1, [self.state[:,0:3], tf.ones([self.particles, 1])])
    # tile here is massively inefficient.
    batch_transformation_matrix = tf.tile(tf.constant([[[1.0,0,0,0], [0,1,0,0], [0,0,1, 0], [0,0,0,1]]]), [self.particles,1,1])
    inp = tf.batch_matmul(tf.expand_dims(inp,1), batch_transformation_matrix)
    # inp is now [batch, 1, 4]
    # w (last index) is scale
    #inp = tf.Print(inp, [inp], summarize=20)
    inp = tf.to_int32(tf.squeeze(inp[:,:,0:2] / inp[:,:,3:4]) * 360 + tf.constant([[240.0, 320]]))
    ys = inp[:,0]
    xs = inp[:,1]

    inp_onscreen = tf.reduce_all([
      tf.greater_equal(ys, tf.constant([0])),
      tf.less(ys, tf.constant([480])),
      tf.greater_equal(xs, tf.constant([0])),
      tf.less(xs, tf.constant([640]))], reduction_indices=[0])
    
    #return (tf.to_int64(tf.concatinate(1, [tf.clip_by_value(ys, 0, 359), tf.clip_by_value(ys, 0, 639)])), inp_onscreen)
    return (inp, inp_onscreen)
    
  def UpdateProbs(self, inp, time_delta):
    """Update probabilities of each particle based on 2D matrix inp which is a 2D perspectiuve projection of the scene"""

    projection, onscreen = self.project()
    filtered_projection = tf.to_int64(tf.select(onscreen, projection, tf.zeros_like(projection)))
    per_state_probabilities = tf.gather_nd(inp, filtered_projection)
    
    filtered_probabilities = tf.select(onscreen, per_state_probabilities, tf.zeros_like(per_state_probabilities))
    
    new_state_indicies = tf.squeeze(tf.multinomial(tf.expand_dims(tf.log(filtered_probabilities),0), self.particles/10*9))
    
    new_state = tf.gather(self.state, new_state_indicies)
    
    # Add momentum
    new_state = tf.concat(1, [new_state[:, 0:3] + new_state[:, 3:6] * time_delta, new_state[:, 3:10]])
    
    # Add in particles for the "just come onscreen" case.
    new_state = tf.concat(0, [new_state, tf.random_normal([self.particles/10, 10]) * self.initial_std + self.initial_bias])

    
    new_state = new_state + tf.random_normal([self.particles, 10]) * self.update_std
    
    
    return self.state.assign(new_state)

  def best_guess(self):
    return tf.reduce_mean(self.state, 0)
    
  def preview_like(self, inp):
    """Generate a preview image of the states of the particle filter"""
    indicies, onscreen = self.project()
    indicies = tf.to_int64(tf.boolean_mask(indicies, onscreen))
    probs = tf.gather_nd(inp, indicies)
    return tf.sparse_tensor_to_dense(tf.sparse_reorder(tf.SparseTensor(indicies, probs, inp.get_shape())), validate_indices = False)
    

