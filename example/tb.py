
import cocotb

print("Hello from TB")

async def my_task():
    print("my_task")
#    print("--> wait")
#    try:
#        await cocotb.triggers.Timer(5, 'ns')
#    except e as Exception:
#        print("Exception: " + str(e))
#    print("<-- wait")
    await cocotb.triggers.Timer(100)
    if False:
        await h

@cocotb.test()
async def my_test(top):
    print("my_test: " + str(top))
    for h in top:
        print("  h: ")
    t = cocotb.fork(my_task())
    print("--> wait 1")
    await cocotb.triggers.Timer(1)
    print("<-- wait 1")
    print("--> wait 2")
    await cocotb.triggers.Timer(1)
    print("<-- wait 2")
    print("--> wait 3")
    await cocotb.triggers.Timer(40)
    print("<-- wait 3")
    print("--> wait 4")
    await cocotb.triggers.Timer(80)
    print("<-- wait 4")
    await cocotb.triggers.Join(t)

#import cocotb;

#async def coro():
#    print("Hello from coro")
#    if False:
#        await oth

#@cocotb.test()
#def test(args):
#    print("Hello from test")
#    cocotb.fork(coro())

