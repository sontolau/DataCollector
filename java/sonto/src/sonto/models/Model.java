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
        public Field pk;
        public Map<String, Field> fields_map = new HashMap<String, Field>();

    };

    public static Meta meta = new Meta();
    public static ModelManager manager = null;

    public static Field[] getFields() {
        return new Field[0];
    }

    /*
     * store the value for each field assigned by constructor or read from
     * table.
     */
    private Map<Field, Object> fv_set = new HashMap<Field, Object>();

    public Model(KeyValue[] kvs) throws Exception {
        for (String f : Model.meta.fields_map.keySet()) {
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

    /* set the value for field. */
    public void set(String f, Object v) throws Exception {
        synchronized (fv_set) {
            Field field = Model.getField(f);
            field.validate(v);
            fv_set.put(field, v);
        }
    }

    /* get value identified by the key. */
    public Object get(String f) throws Exception {
        Field field = Model.getField(f);

        synchronized (fv_set) {
            return fv_set.get(field);
        }
    }

    /* save to database. */
    public void save(boolean update) throws Exception {
        if (update) {
            this.manager.db.update(this);
        } else {
            this.manager.db.insert(this);
        }
    }


    /* delete row from database. */
    public void delete() {
        this.manager.db.delete (this);
    }

    /****** STATIC METHOD ******/

    public static void dumpFields() {
        for (String k : Model.meta.fields_map.keySet()) {
            System.out.println(k);
        }
    }

    public static boolean hasField(String f) {
        synchronized (Model.meta.fields_map) {
            return Model.meta.fields_map.containsKey(f);
        }
    }

    public static Field getField(String f) throws NoFieldFoundException {
        synchronized (Model.meta.fields_map) {
            if (!hasField(f)) {
                throw new NoFieldFoundException();
            }

            return Model.meta.fields_map.get(f);
        }
    }
}
