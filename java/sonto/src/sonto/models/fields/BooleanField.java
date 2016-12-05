package sonto.models.fields;

import sonto.exception.InvalidValueTypeException;

public class BooleanField extends Field {

    public BooleanField(String name, FieldAttribute[] attrs) {
        super(name, attrs);
        // TODO Auto-generated constructor stub
    }
    
    @Override
    public void validate(Object v) throws InvalidValueTypeException {
        if (!(v instanceof Boolean)) {
            throw new InvalidValueTypeException ();
        }
    }
}
