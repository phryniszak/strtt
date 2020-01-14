module.exports = {
    // http://eslint.org/docs/rules/
    "env": {
        "node": true,
        "browser": true,
        "es6": true
    },
    "extends": "eslint:recommended",
    "rules": {
        "indent": [
            "error",
            4
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
        "$": true,
        "jQuery": true,
        "ReconnectingWebSocket": true,
        "echarts": true
    }
};