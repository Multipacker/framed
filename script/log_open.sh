#!/bin/sh
pane=$(tmux list-panes -F "#{pane_current_command}[#{pane_id}]" | sed -nE 's/vim.*\[(.*)\]$/\1/p')
tmux send-keys -t $pane ":e +$2 $1" Enter && tmux select-pane -t $pane || tmux split vim +$2 $1
