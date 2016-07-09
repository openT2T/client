package io.opent2t;

import java.util.EventListener;

public interface NodeCallListener extends EventListener {
    void functionCalled(NodeCallEvent e);
}
