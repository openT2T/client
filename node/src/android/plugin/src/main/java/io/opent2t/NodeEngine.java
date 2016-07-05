package io.opent2t;

import android.util.Log;

import java.util.HashSet;
import java.util.concurrent.Future;

public class NodeEngine {
    private static final String TAG = "OpenT2T.NodeEngine";

    static
    {
        System.loadLibrary("NodeEngine");
        NodeEngine.staticInit();
    }

    private static native void staticInit();

    /**
     * Native pointer to the node engine instance.
      */
    private long node;

    private HashSet<NodeCallListener> callFromScriptListeners;

    public NodeEngine() {
        this.init();
    }

    private native void init();

    public native String getMainScriptFileName();

    public native void defineScriptFile(String scriptFileName, String scriptCode);

    public Future<Void> startAsync(String workingDirectory) {
        Promise<Void> promise = new Promise<Void>();
        this.start(promise, workingDirectory);
        return promise;
    }

    private native void start(Promise promise, String workingDirectory);

    public Future<Void> stopAsync() {
        Promise<Void> promise = new Promise<Void>();
        this.stop(promise);
        return promise;
    }

    private native void stop(Promise promise);

    public Future<String> callScriptAsync(String scriptCode) {
        Promise<String> promise = new Promise<String>();
        this.callScript(promise, scriptCode);
        return promise;
    }

    private native void callScript(Promise promise, String scriptCode);

    public native void registerCallFromScript(String scriptFunctionName);

    public synchronized void addCallFromScriptListener(NodeCallListener listener) {
        this.callFromScriptListeners.add(listener);
    }

    public synchronized void removeCallFromScriptListener(NodeCallListener listener) {
        this.callFromScriptListeners.remove(listener);
    }

    private synchronized void raiseCallFromScript(String functionName, String argsJson) {
        for (NodeCallListener listener : this.callFromScriptListeners) {
            try {
                listener.functionCalled(new NodeCallEvent(this, functionName, argsJson));
            }
            catch (Exception ex) {
                Log.e(TAG, "Exception in CallFromScript listener", ex);
            }
        }
    }
}
