/**
 * Attach controls to given dag animator.
 */
function Control(animator) {
    "use strict";
    var play_src = document.querySelector("#hidden_icon > img").src,
        playpause_img = document.querySelector("#playpause > img"),
        pause_src = playpause_img.src,
        ts = document.querySelector("#timestep"),
        as = document.querySelector("#autoscroll");
    function pause() {
        // Pause the animation and show the play button.
        playpause_img.src = play_src;
        animator.pause();
    }
    function unpause() {
        // Unpause animation and show pause button.
        animator.unpause();
        playpause_img.src = pause_src;
    }
    function togglePlay() {
        // Handles pressing the play/pause button
        if (animator.paused()) {
            unpause();
            animator.showInOrder();
        } else {
            pause();
        }
    }
    function toStart() {
        // Pause animation and hide all elements.
        pause();
        animator.hideAll();
    }
    function toEnd() {
        // Pause animation and show all elements.
        pause();
        animator.showAll();
    }
    function prev() {
        // Pause animation and hide the last shown node.
        pause();
        animator.hidePrev();
    }
    function next() {
        // Pause animation and show the next node.
        pause();
        animator.showNext();
    }
    document.querySelector("#start").addEventListener('click', toStart);
    document.querySelector("#prev").addEventListener('click', prev);
    document.querySelector("#playpause").addEventListener('click', togglePlay);
    document.querySelector("#next").addEventListener('click', next);
    document.querySelector("#end").addEventListener('click', toEnd);
    // the timestep slider is labeled "speed" but we want "delay"
    // so we subtract from max to get the timestep value
    ts.addEventListener('change', function() {
        animator.setTimestep(parseInt(ts.max)+1-parseInt(ts.value));
    });
    as.addEventListener('click', function() {
        animator.setAutoscroll(as.checked);
    });

    window.addEventListener('keypress', function(event) {
        // Bind p to play/pause, [ ] to prev/next, < > to start/end.
        var k = (event.keyCode ? event.keyCode : event.which);
        if (k === 112)            // p
            togglePlay();
        else if (k === 91)        // [
            prev();
        else if (k === 93)        // ]
            next();
        else if (k === 60)        // <
            toStart();
        else if (k === 62)        // >
            toEnd();
        if (k === 112 || k === 91 || k === 93 || k === 60 || k === 62)
            event.stopPropagation();
    });
    ts.value = parseInt(ts.max) - animator.getTimestep();
}
