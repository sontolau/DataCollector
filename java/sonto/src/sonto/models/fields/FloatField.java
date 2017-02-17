package sonto.models.fields;

import sonto.exception.InvalidValueTypeException;

public class FloatField extends Field {
    public FloatField(String name, FieldAttribute[] attrs) {
        super(name, attrs);
        // TODO Auto-generated constructor stub
    }
    
    public void validate(Object v) throws InvalidValueTypeException {
        if (!(v instanceof Float)) {
            throw new InvalidValueTypeException ();
        }
    }
}
