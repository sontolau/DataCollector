package sonto.models;

import java.util.HashMap;
import java.util.Map;

import sonto.common.KeyValue;
import sonto.exception.InvalidValueTypeException;
import sonto.exception.NoFieldFoundException;
import sonto.exception.NotAttributeFoundException;
import sonto.exception.NotNullFieldException;
import sonto.exception.OutOfBoundFieldException;
import sonto.exception.UnknownFieldException;
import sonto.models.fields.CharField;
import sonto.models.fields.Field;
import sonto.models.fields.Field.FieldAttribute;

public abstract class Model {
    public static class Meta {
        public String table;
        public Map<String, Field> fields_map = new HashMap<String, Field> ();
        
    };
    
    public static String table = null;
    public static Meta meta = new Meta();
    public static Map<String, Field> fields = null;
    public static Field pk = null;
    public static ModelManager manager = null;
    public static Field[] getFields() {
        return new Field[0];
    }

    private Map<Field, Object> fv_set = new HashMap<Field, Object>();
    
    public Model(KeyValue[] kvs) throws Exception {
        for (String f : Model.fields.keySet()) {
            Field field = Model.getField(f);
            FieldAttribute attr = null;
            
            if (field.getAttribute("isNull").test(true)) {
                continue;
            }
            
            if (!(attr = field.getAttribute("default")).isNull()) {
                field.validate(attr.value);
                fv_set.put(field, attr.value);
            }
        }
        
        for (KeyValue kv : kvs) {
            this.set(kv.key, kv.value);
        }
    }

    public void set(String f, Object v) throws Exception {
        synchronized (fv_set) {
            Field field = Model.getField(f);
            field.validate(v);
            fv_set.put(field, v);
        }
    }
    
    
    public void save(boolean update) throws Exception {
//        
//        if (update) {
//            this.manager.db.update(this);
//        } else {
//            this.manager.db.insert(this);
//        }
    }

    public Object getFieldValue(String field) throws NoFieldFoundException {
        Field f = Model.getField(field);
        
        synchronized (fv_set) {
            return fv_set.get(f);
        }
    }
   
    public void delete() {
        
    }

    /****** STATIC METHOD ******/
    
    public static void dumpFields() {
        for (String k : fields.keySet()) {
            System.out.println (k);
        }
    }
    
    public static boolean hasField (String f) {
        synchronized (fields) {
            return fields.containsKey(f);
        }
    }
    
    public static Field getField(String f) throws NoFieldFoundException {
        synchronized (fields) {
            if (!hasField (f)) {
                throw new NoFieldFoundException ();
            }
            
            return fields.get(f);
        }
    }
}
