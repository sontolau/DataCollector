package sonto.common;


public abstract class KeyValue {
    public String key;
    public Object value;
    
    public KeyValue () {
        
    }
    
    public KeyValue (String key, Object v) {
        this.key = key;
        this.value = v;
    }
    
    public KeyValue (String key, int v) {
        this (key, new Integer (v));
    }
    
    public KeyValue (String key, boolean v) {
        this (key, new Boolean (v));
    }
    
    public KeyValue (String key, float f) {
        this (key, new Float (f));
    }
}
