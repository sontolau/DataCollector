package sonto.models;

import sonto.common.KeyValue;
import sonto.models.fields.Field;

public class Filter{
    public static enum Operator {
        LT, EQ, GT, NOT, LIKE
    };
    
    public Field field;
    public Operator op;
    public Object value;
    
    public Filter (Field f, Operator op, Object v) {
        this.field = f;
        this.op = op;
        this.value = v;
    }
}
