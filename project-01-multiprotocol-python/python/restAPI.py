from flask import Flask, jsonify, request

def create_app(connection):
    app = Flask(__name__)

    @app.route('/variables', methods=['GET'])
    def get_variables():
        variables = connection.read_variables_all()
        return jsonify(variables)

    @app.route('/variable/<path:topic>', methods=['GET'])
    def get_variable(topic):
        variables = connection.read_variables_all()
        for var in variables:
            if var["Topic"] == topic:
                return jsonify(var)
        return jsonify({"error": "Variable not found"}), 404

    @app.route('/write', methods=['POST'])
    def write_variable():
        data = request.json
        topic = data["topic"]
        value = data["value"]
        
        if not connection.is_writable(topic):
            return jsonify({"error": f"Variable {topic} is read-only"}), 403
        
        connection.write_variable(topic, value)
        return jsonify({"status": "ok"})

    return app