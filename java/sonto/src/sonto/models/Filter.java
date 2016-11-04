package sonto.models;

import sonto.common.KeyValue;
import sonto.models.fields.Field;

public class Filter extends KeyValue{
    public static enum Operator {
        LT, EQ, GT, NOT, LIKE
    };
    
    public Operator op;

    public Filter (String f, Operator op, Object v) {
        super(f, v);
        this.op = op;
    }
}
