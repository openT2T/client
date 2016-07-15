package io.opent2t;

import java.util.EventListener;

/**
 * Listens for calls to functions registered in the node scripting environment.
 */
public interface NodeCallListener extends EventListener {
    void functionCalled(NodeCallEvent e);
}
