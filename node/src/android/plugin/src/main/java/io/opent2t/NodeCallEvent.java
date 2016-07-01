package io.opent2t;

import java.util.EventObject;

public class NodeCallEvent extends EventObject {

    private String functionName;
    private String argsJson;

    NodeCallEvent(Object source, String functionName, String argsJson) {
        super(source);
        this.functionName = functionName;
        this.argsJson = argsJson;
    }

    public String getFunctionName() {
        return this.functionName;
    }

    public String getArgsJson() {
        return this.argsJson;
    }
}
