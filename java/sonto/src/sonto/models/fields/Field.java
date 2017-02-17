package sonto.models.fields;

import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Map;

import sonto.common.KeyValue;
import sonto.exception.InvalidClassException;
import sonto.exception.InvalidFieldAttributeException;
import sonto.exception.InvalidValueTypeException;
import sonto.exception.NotAttributeFoundException;
import sonto.models.Model;

public abstract class Field {

    public static enum AttributeType {
        BOOL, INT, FLOAT, STRING, UNKNOWN,
    };

    public static class FieldAttribute extends KeyValue {
        public AttributeType type = AttributeType.UNKNOWN;

        public FieldAttribute(String key, AttributeType type, Object o) {
            super(key, o);
            this.type = type;
        }

        public FieldAttribute(String key, boolean v) {
            this(key, AttributeType.BOOL, v);
            // TODO Auto-generated constructor stub
        }

        public FieldAttribute(String key, int v) {
            this(key, AttributeType.INT, v);
            // TODO Auto-generated constructor stub
        }

        public FieldAttribute(String key, float v) {
            this(key, AttributeType.FLOAT, v);
            // TODO Auto-generated constructor stub
        }

        public FieldAttribute(String key, String s) {
            this(key, AttributeType.STRING, s);
        }

        public boolean test(boolean b) {
            return (b == ((Boolean) this.value).booleanValue());
        }

        public boolean test(int b) {
            return (b == ((Integer) this.value).intValue());
        }

        public boolean test(float b) {
            return (b == ((Float) this.value).floatValue());
        }

        public boolean test(String s) {
            return s.equals((String) this.value);
        }

        public boolean isNull() {
            return (this.value == null);
        }
    }

    // field name.
    public String name;
    protected Map<String, FieldAttribute> attributes = new HashMap<String, FieldAttribute>();

    public Field(String name) {
        this.name = name;

        FieldAttribute attrs[] = new FieldAttribute[] {
                new FieldAttribute("isPrimaryKey", false),
                new FieldAttribute("isUniqure", false),
                new FieldAttribute("comment", AttributeType.STRING, null),
                new FieldAttribute("isNull", false),
                new FieldAttribute("columnName", AttributeType.STRING, name),
                new FieldAttribute("default", AttributeType.UNKNOWN, null), };

        for (FieldAttribute attr : attrs) {
            this.addAttribute(attr);
        }
    }

    protected void addAttribute(FieldAttribute attr) {
        synchronized (attributes) {
            attributes.put(attr.key, attr);
        }
    }

    public Field(String name, FieldAttribute[] attrs) {
        this(name);
        if (attrs != null) {
            for (FieldAttribute attr : attrs) {
                this.addAttribute(attr);
            }
        }
    }

    public boolean hasAttribute(String attr) {
        synchronized (attributes) {
            return this.attributes.containsKey(attr);
        }
    }

    public FieldAttribute getAttribute(String attr)
            throws NotAttributeFoundException {
        synchronized (attributes) {
            if (this.attributes.containsKey(attr)) {
                return ((FieldAttribute) this.attributes.get(attr));
            }
        }
        throw new NotAttributeFoundException();
    }

    public void validate(Object v) throws Exception {

    }
}
