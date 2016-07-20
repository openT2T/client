package io.opent2t;

import java.util.EventObject;

/**
 * Event raised when a registered function is called by script.
 */
public class NodeCallEvent extends EventObject {

    private String functionName;
    private String argsJson;

    NodeCallEvent(Object source, String functionName, String argsJson) {
        super(source);
        this.functionName = functionName;
        this.argsJson = argsJson;
    }

    /**
     * Name of the function that was called by script.
     */
    public String getFunctionName() {
        return this.functionName;
    }

    /**
     * JSON-serialized array of arguments passed by the script.
     */
    public String getArgsJson() {
        return this.argsJson;
    }
}
