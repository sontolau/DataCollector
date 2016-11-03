package sonto.models;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

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

    private static void testModel(Class<?> model) throws InvalidClassException {
        if (!model.getSuperclass().equals (Model.class)) {
            throw new InvalidClassException();
        }
    }
    
    public static void addModel(Class<?> model) throws Exception {
        synchronized (models) {
            int i = 0;
            
            if (!model.getSuperclass().equals(Model.class)) {
                throw new InvalidClassException();
            }
            
            Map<String, Field> map = new HashMap<String, Field>();
            
//            Method get_fields = model.getMethod("getFields", null);
//            java.lang.reflect.Field pk = model.getField("pk");
            java.lang.reflect.Field meta = model.getField ("meta");
            Meta meta_info = (Meta)meta.get(model);
            
            java.lang.reflect.Field fields = model.getField("db_fields");
            java.lang.reflect.Field table_name = model.getField("db_table");
            
            if (table_name != null) {
                meta_info.table = (String)table_name.get(model);
            }
            
            
            
            
            
            System.out.println((String)table_name.get(model));
            
           
            

            
                    
//            Field[] sfields = (Field[]) get_fields.invoke(null, null);

            pk.set(model, new AutoField("id", 1, 1,
                    new FieldAttribute[] {new FieldAttribute("isPrimaryKey",true)}));
//            
//            for (Field f : sfields) {
//                if (f.getAttribute("isPrimaryKey").test(true)) {
//                    pk.set(model, f);
//                }
//                map.put(f.name, f);
//            }

            fields.set(model, map);
            
            java.lang.reflect.Field manager = model.getField("manager");
            manager.set(model, new ModelManager(model));

            models.add(model);
        }
    }

    
    public static void setSettings(Settings cls) throws NoSuchFieldException,
            SecurityException, InstantiationException, IllegalAccessException {
//        if (cls.db_engine.toLowerCase().equals("mysql")) {
//            db = new Mysql(cls.db_host, cls.db_port, cls.db_name, cls.db_user,
//                    cls.db_password);
//        }
    }
}
