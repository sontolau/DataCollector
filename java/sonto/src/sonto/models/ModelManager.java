package sonto.models;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import sonto.common.KeyValue;
import sonto.db.DBIface;
import sonto.db.impl.Mysql;
import sonto.exception.InvalidClassException;
import sonto.models.fields.AutoField;
import sonto.models.fields.Field;
import sonto.models.fields.Field.FieldAttribute;
import sonto.models.Model.Meta;
import sonto.models.Settings;

public class ModelManager {
    private final static List<Class<?>> models = new ArrayList<Class<?>>();
    public Class<?> model = null;
    public static DBIface db = null;

    public ModelManager(Class<?> model) {
        this.model = model;
    }
    
//    public Model[] filter(Filter[] filters) {
//        return null;
//    }
    
    public Model get_or_create (KeyValue[] kvs) {
        return null;
    }

    /*register a model, this method is mandatory before using model to process database.*/
    public static void registerModel(Class<?> model) throws Exception {
        synchronized (models) {
            if (!model.getSuperclass().equals(Model.class)) {
                throw new InvalidClassException();
            }

            java.lang.reflect.Field meta = model.getField("meta");
            Meta meta_info = (Meta) meta.get(model);

            /* new a default primary key for model */
            meta_info.pk = new AutoField("id", 1, 1,
                    new FieldAttribute[] { new FieldAttribute("isPrimaryKey",
                            true) });

            /* test if the table properties are defined. */
            java.lang.reflect.Field fields = model.getField("db_fields");
            java.lang.reflect.Field table_name = model.getField("db_table");

            if (table_name != null) {
                meta_info.table = (String) table_name.get(model);
            }

            if (fields != null) {
                Field[] sfields = (Field[]) fields.get(model);
                for (Field f : sfields) {
                    if (f.getAttribute("isPrimaryKey").test(true)) {
                        meta_info.pk = f;
                    }

                    meta_info.fields_map.put(f.name, f);
                }
            }
            
            /* new a model manager for each model. */
            java.lang.reflect.Field manager = model.getField("manager");
            manager.set(model, new ModelManager(model));
            models.add(model);
        }
    }

    public static void setSettings(Settings cls) throws NoSuchFieldException,
            SecurityException, InstantiationException, IllegalAccessException {
        // if (cls.db_engine.toLowerCase().equals("mysql")) {
        // db = new Mysql(cls.db_host, cls.db_port, cls.db_name, cls.db_user,
        // cls.db_password);
        // }
    }
}
