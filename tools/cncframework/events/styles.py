# -*- coding: utf-8 -*-
import random

# here we define some shape, color, style maps
# default color is black
_colors = (
{
    'step': 'blue',
    'item': ['seagreen', 'green', 'springgreen', 'turquoise', 'limegreen', 'lawngreen'],
    'get_without_put': 'firebrick',
    'put_without_get': 'orchid',
    'prescribe_without_run': 'hotpink',
})

# default shape is an ellipse
_shapes = ({
    'item': 'box',
})

# default style is solid
_styles = ({
    'prescribe': 'dashed',
})

# make sure we always return same color for given key
_color_key_cache = {}
def color(event, key=None):
    """
    Return the color for the requested event or node type.

    Key is required for events with multiple possible colors.
    The result of a given (event,key) pair is cached and the same result is
    returned each time, even though the first choice is randomized.
    Default color: black.
    """
    c = _colors.get(event, 'black')
    if type(c) == str:
        return c
    if not key:
        raise KeyError("Expected a key to choose color.")
    # randomly pick a color if we haven't seen this event,key pair
    # otherwise return what we saw last time
    choice = _color_key_cache.get((event,key), random.choice(c))
    _color_key_cache[(event,key)] = choice
    return choice

def shape(node_type):
    """
    Return a shape string for the given node type.

    Default shape: ellipse.
    """
    return _shapes.get(node_type, 'ellipse')

def style(edge_type):
    """
    Return a style string for the given edge type.

    Default style: solid.
    """
    return _styles.get(edge_type, 'solid')
