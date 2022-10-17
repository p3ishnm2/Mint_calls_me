import json
import boto3


def call_from_garden(event):
    Bat = event['Bat']
    temp = event['temp']
    # hum = event['hum']
    moisture = event['moisture']
    # CO2 = event['CO2']

    # サービスクォータに問い合わせて，解除されるまではコメントアウト
    # region_nameは同じus-east1なので指定する必要はないはず
    connect = boto3.client('connect')
    if moisture < 70:
        message = '…もしもし，ミントです。'
        message_thirsty = 'そろそろ喉乾いたんだけど、お水ちょうだい。'
        message = message + message_thirsty
        flag_moist = 1
    if temp > 29:
        if moisture < 70:
            message = message + 'あと、'
        message_warm = '部屋が暑くなってきたから早く帰ってきて，エアコンつけてよ。'
        message = message + message_warm
        flag_temp = 1
    connect.start_outbound_voice_contact(
        DestinationPhoneNumber='',
        ContactFlowId='',
        InstanceId='',
        SourcePhoneNumber='',
        Attributes={
            'message': message
        }
    )
    # 一回電話かけたらしばらくはかけないようにする
    # 水やりしたらお礼を言うようにする


def lambda_handler(event, context):
    print(event)
    Bat = event['Bat']
    temp = event['temp']
    # hum = event['hum']
    moisture = event['moisture']
    # CO2 = event['CO2']

    if moisture < 70 or temp > 28:
        call_from_garden(event)
    # TODO implement
    # eventからパージする
    # return {
    #     'statusCode': 200,
    #     'body': json.dumps('Hello from Lambda!')
    # }
