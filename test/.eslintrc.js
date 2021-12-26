module.exports = {
    // http://eslint.org/docs/rules/
    "env": {
        "node": true,
        "es6": true,
        "browser": false
    },
    "parserOptions": {
        "ecmaVersion": 2019,
        "sourceType": "module"
    },
    "extends": "eslint:recommended",
    "rules": {
        "indent": [
            "error",
            4,
            { "SwitchCase": 1 }
        ],
        "linebreak-style": [
            "error",
            "unix"
        ],
        "quotes": [
            "error",
            "double"
        ],
        "semi": [
            "error",
            "always"
        ],
        "no-console": 0
    },
    "globals": {
        "appRoot": "readonly"
    }
};