import dash
from dash import dcc, html
from dash.dependencies import Input, Output
import plotly.graph_objs as go
import pandas as pd
import sqlite3

# Initialize the Dash app
app = dash.Dash(__name__)

# Define the layout of the app with three Graph components
app.layout = html.Div(children=[
    html.H1(children='Real-time Data'),
    dcc.Graph(id='acc-graph'),
    dcc.Graph(id='gyro-graph'),
    dcc.Graph(id='ypr-graph'),
    dcc.Interval(
        id='interval-component',
        interval=1*500,  # in milliseconds, updated to 1000 for 1 second intervals
        n_intervals=0
    )
])

# Callback function to update the YPR graph
@app.callback(Output('ypr-graph', 'figure'),
              [Input('interval-component', 'n_intervals')])
def update_ypr_live(n):
    return update_graph_live('ypr')

# Callback function to update the Acceleration graph
@app.callback(Output('acc-graph', 'figure'),
              [Input('interval-component', 'n_intervals')])
def update_acc_live(n):
    return update_graph_live('acc')

# Callback function to update the Gyroscope graph
@app.callback(Output('gyro-graph', 'figure'),
              [Input('interval-component', 'n_intervals')])
def update_gyro_live(n):
    return update_graph_live('gyro')

def update_graph_live(data_type):
    # Connect to the SQLite database
    conn = sqlite3.connect('data/sensor_data.db')
    # Adjusted query to select different types of data based on the data_type argument
    if data_type == 'ypr':
        select_columns = 'timestamp, yaw, pitch, roll'
        title = 'Yaw, Pitch, and Roll Over the Last 30 Seconds'
        yaxis_range = [-200, 200]
    elif data_type == 'acc':
        select_columns = 'timestamp, acc_x, acc_y, acc_z'
        title = 'Acceleration Over the Last 30 Seconds'
        yaxis_range = [-2, 2]
    elif data_type == 'gyro':
        select_columns = 'timestamp, gyro_x, gyro_y, gyro_z'
        title = 'Gyroscope Over the Last 30 Seconds'
        yaxis_range = [-500, 500]
    
    query = f"""
    SELECT {select_columns} FROM orientation
    WHERE timestamp >= (SELECT MAX(timestamp) - 30 * 1000000 FROM orientation)
    ORDER BY timestamp ASC
    """
    df = pd.read_sql_query(query, conn)
    conn.close()

    # Convert timestamp from microseconds to seconds for plotting
    df['timestamp'] = pd.to_datetime(df['timestamp'], unit='us')

    # Create traces based on the data_type
    if data_type in ['ypr', 'acc', 'gyro']:
        traces = []
        for column in df.columns[1:]:  # Skip the timestamp column
            traces.append(go.Scatter(
                x=df['timestamp'],
                y=df[column],
                name=column.upper(),
                mode='lines'
            ))

    # Define the figure
    fig = {'data': traces, 'layout': go.Layout(title=title, xaxis_title='Time', yaxis=dict(title='Value', range=yaxis_range))}

    return fig

# Run the app
if __name__ == '__main__':
    app.run_server(debug=True)
